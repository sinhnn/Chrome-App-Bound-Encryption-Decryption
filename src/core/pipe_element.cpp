#include "pipe_element.hpp"
#include "logger.hpp"
#include <cstring>
#include <stdexcept>
#include "../core/common.hpp"

namespace Core
{

    const char kHeaderSign[] = {'_', '_', 'S', 'O', 'P'}; // Example header signature for packets
    const char kFooterSign[] = {'E', 'O', 'P', '_', '_'}; // Example footer signature for packets
    const int kCrcSize = 4; // CRC32

    PipeElement::PipeElement(HANDLE hFile, Core::Logger& logger)
        : hFile_(hFile), logger_(logger)
    {
        // if (hFile_ == nullptr || hFile_ == INVALID_HANDLE_VALUE)
        // {
        //     throw std::invalid_argument("Invalid pipe handle");
        // }
    }

    PipeElement::~PipeElement()
    {
        if (hFile_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
        }
    }

    int PipeElement::close()
    {
        if (hFile_ != INVALID_HANDLE_VALUE) {
            logger_.info(L"Sending close tunnel message");
            write(kMessageType_CloseTunnel, Core::PipeElement::kCloseTunnelMsg, sizeof(Core::PipeElement::kCloseTunnelMsg));
            logger_.info(L"Closing tunnel");
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            logger_.info(L"Tunnel closed");
        }
        return 0;
    }

    bool PipeElement::is_valid() const
    {
        return hFile_ != INVALID_HANDLE_VALUE;
    }

    int PipeElement::write_ (const char* buffer, uint32_t size, DWORD& bytesWritten)
    {
        int retcode = 0;
        bytesWritten = 0;
        // DWORD bytesWritten;

        logger_.info(L"TX: " + Core::KeyToHexW(reinterpret_cast<const wchar_t *>(buffer), size / sizeof(wchar_t)));
        if (!WriteFile(hFile_, buffer, static_cast<DWORD>(size), &bytesWritten, nullptr))
        {
            return -1; // Error
        }
        return retcode;
    }

    int PipeElement::write(int message_type, const char* buffer, uint32_t size)
    {
        int retcode = 0;
        DWORD bytesWritten;
        char packedBuffer[kBufferSize]; // Temporary buffer for packing the data
        uint32_t packedSize = 0;
        if (pack_to(message_type, buffer, size, packedBuffer, packedSize) != 0)
        {
            return -1; // Error packing the data
        }

        if (packedSize == 0)
        {
            return -1; // Error: packed size is zero
        }
        return write_(packedBuffer, packedSize, bytesWritten);
    }

    int PipeElement::write(int message_type, int command_id, const char* buffer, uint32_t size)
    {
        int retcode = 0;
        DWORD bytesWritten;
        char packedBuffer[kBufferSize]; // Temporary buffer for packing the data
        uint32_t packedSize = 0;
        if (pack_to(message_type, command_id, buffer, size, packedBuffer, packedSize) != 0)
        {
            return -1; // Error packing the data
        }

        if (packedSize == 0)
        {
            return -1; // Error: packed size is zero
        }

        return write_(packedBuffer, packedSize, bytesWritten);
    }

    int PipeElement::write(int message_type, int command_id, std::vector<std::vector<char>> buffers)
    {
        int retcode = 0;
        DWORD bytesWritten;
        char packedBuffer[kBufferSize]; // Temporary buffer for packing the data
        uint32_t packedSize = 0;
        if (pack_to(message_type, command_id, buffers, packedBuffer, packedSize) != 0)
        {
            return -1; // Error packing the data
        }

        if (packedSize == 0)
        {
            return -1; // Error: packed size is zero
        }

        return write_(packedBuffer, packedSize, bytesWritten);
    }


    int PipeElement::send_text(std::wstring message)
    {
        logger_.info(L"Sending TEXT: " + message);
        return write(kMessageType_Text, reinterpret_cast<const char*>(message.data()), static_cast<uint32_t>(message.size() * sizeof(wchar_t)));
    }

    int PipeElement::register_handler(int message_type, std::function<void(const char* buffer, uint32_t size)> handler)
    {
        if (!handler)
        {
            return -1; // Error: handler is null
        }

        if (handlers_.find(message_type) != handlers_.end())
        {
            return -2; // Error: handler for this message type already exists
        }

        handlers_[message_type] = handler;
        return 0; // Success
    }

    int PipeElement::unregister_handler(int message_type)
    {
        if (handlers_.find(message_type) == handlers_.end())
        {
            return -1; // Error: handler for this message type does not exist
        }

        handlers_.erase(message_type);
        return 0; // Success
    }

    int PipeElement::read_utils(std::function<bool(int msg_type, const char* buffer, uint32_t size)> delegate, uint32_t timeout_ms)
    {
        DWORD bytesRead;
        char localBuffer[kBufferSize]; // Temporary buffer for reading
        size_t now = GetTickCount64();
        size_t end_time = now + timeout_ms;
        int retcode = -1;

        while (GetTickCount64() < end_time)
        {
            DWORD available = 0;
            if (!PeekNamedPipe(hFile_, nullptr, 0, nullptr, &available, nullptr))
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    logger_.error(L"Pipe broken");
                    break;
                }

                throw std::runtime_error("PeekNamedPipe failed");
                return -1; // Error
            }

            if (available == 0)
            {
                Sleep(kSleepIntervalMs); // Sleep for a short while before checking again
                continue; // No data available, keep waiting
            }

            if (!ReadFile(hFile_, localBuffer, sizeof(localBuffer), &bytesRead, nullptr))
            {
                logger_.error(L"ReadFile failed");
                return -1; // Error
            }

            retcode = is_valid_packet(localBuffer, static_cast<uint32_t>(bytesRead));
            if (retcode != kSuccessCode)
            {
                logger_.error(L"Invalid packet received retcode=" + std::to_wstring(retcode));
                break;
            }

            // Unpack the packet to the input buffer

            uint32_t bytesMsg = bytesRead - sizeof(kHeaderSign) - sizeof(uint32_t) - sizeof(kFooterSign); // Subtract header and size field
            logger_.info(L"RX: " + Core::KeyToHexW(reinterpret_cast<const wchar_t *>(localBuffer + sizeof(kHeaderSign)), bytesMsg / sizeof(wchar_t)));

            int message_type = *reinterpret_cast<int*>(localBuffer + sizeof(kHeaderSign));
            char* buffer = localBuffer + sizeof(kHeaderSign) + sizeof(int);
            if (message_type == kMessageType_CloseTunnel)
            {
                logger_.info(L"Close tunnel message received");
                retcode = -1;
                break;
            }

            size_t size = bytesRead - (sizeof(kHeaderSign) + sizeof(int) + kCrcSize + sizeof(kFooterSign));
            if (delegate(message_type, buffer, static_cast<uint32_t>(size)))
            {
                retcode = 0;
                break;
            }
        }

        return retcode; // Timeout or delegate returned false
    }


    int PipeElement::pack_to(int message_type, const char* buffer, uint32_t size, char* packed_buffer, uint32_t& packed_size) {
        if (!packed_buffer) {
            return -1; // Invalid buffer
        }

        packed_size = 0;

        size_t total_size = sizeof(kHeaderSign) + sizeof(int) + size + kCrcSize + sizeof(kFooterSign);
        size_t offset = 0;

        // Copy header
        memcpy(packed_buffer + offset, kHeaderSign, sizeof(kHeaderSign));
        offset += sizeof(kHeaderSign);

        // Copy message type
        memcpy(packed_buffer + offset, &message_type, sizeof(int));
        offset += sizeof(int);

        // Copy payload
        memcpy(packed_buffer + offset, buffer, size);
        offset += size;

        // Compute and copy CRC (dummy CRC for now)
        uint32_t crc = 0; // Replace with actual CRC computation
        memcpy(packed_buffer + offset, &crc, kCrcSize);
        offset += kCrcSize;

        // Copy footer
        memcpy(packed_buffer + offset, kFooterSign, sizeof(kFooterSign));
        offset += sizeof(kFooterSign);

        packed_size = static_cast<uint32_t>(total_size);
        return 0;
    }

    int PipeElement::pack_to(int message_type,
            int command_id,
            const char* buffer,
            uint32_t size, char* packed_buffer, uint32_t& packed_size) {
        if (!packed_buffer) {
            return -1; // Invalid buffer
        }

        packed_size = 0;

        size_t total_size = sizeof(kHeaderSign) + sizeof(int) + sizeof(int) + size + kCrcSize + sizeof(kFooterSign);
        size_t offset = 0;

        // Copy header
        memcpy(packed_buffer + offset, kHeaderSign, sizeof(kHeaderSign));
        offset += sizeof(kHeaderSign);

        // Copy message type
        memcpy(packed_buffer + offset, &message_type, sizeof(int));
        offset += sizeof(int);

        // Copy command ID
        memcpy(packed_buffer + offset, &command_id, sizeof(int));
        offset += sizeof(int);

        // Copy payload
        memcpy(packed_buffer + offset, buffer, size);
        offset += size;

        // Compute and copy CRC (dummy CRC for now)
        uint32_t crc = 0; // Replace with actual CRC computation
        memcpy(packed_buffer + offset, &crc, kCrcSize);
        offset += kCrcSize;

        // Copy footer
        memcpy(packed_buffer + offset, kFooterSign, sizeof(kFooterSign));
        offset += sizeof(kFooterSign);

        packed_size = static_cast<uint32_t>(total_size);
        return 0;
    }

    int PipeElement::pack_to(int message_type,
            int command_id,
            std::vector<std::vector<char>> buffers,
            char* packed_buffer,
            uint32_t& packed_size) {
        if (!packed_buffer) {
            return -1; // Invalid buffer
        }

        packed_size = 0;

        size_t total_size = sizeof(kHeaderSign) + sizeof(int) + sizeof(int);
        for (const auto& buf : buffers) {
            total_size += buf.size();
        }
        total_size += kCrcSize + sizeof(kFooterSign);

        size_t offset = 0;

        // Copy header
        memcpy(packed_buffer + offset, kHeaderSign, sizeof(kHeaderSign));
        offset += sizeof(kHeaderSign);

        // Copy message type
        memcpy(packed_buffer + offset, &message_type, sizeof(int));
        offset += sizeof(int);

        // Copy command ID
        memcpy(packed_buffer + offset, &command_id, sizeof(int));
        offset += sizeof(int);

        // Copy payloads
        for (const auto& buf : buffers) {
            uint32_t buf_size = static_cast<uint32_t>(buf.size());
            memcpy(packed_buffer + offset, &buf_size, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            memcpy(packed_buffer + offset, buf.data(), buf_size);
            offset += buf_size;
        }

        // Compute and copy CRC (dummy CRC for now)
        uint32_t crc = 0; // Replace with actual CRC computation
        memcpy(packed_buffer + offset, &crc, kCrcSize);
        offset += kCrcSize;

        // Copy footer
        memcpy(packed_buffer + offset, kFooterSign, sizeof(kFooterSign));
        offset += sizeof(kFooterSign);

        packed_size = static_cast<uint32_t>(total_size);
        return 0;
    }

    int PipeElement::pack_to(int message_type, std::vector<std::vector<char>> buffers, char* out_buffer, uint32_t& out_size) {
        if (!out_buffer) {
            return -1; // Invalid buffer
        }

        out_size = 0;

        size_t total_size = sizeof(kHeaderSign) + sizeof(int);
        for (const auto& buf : buffers) {
            total_size += buf.size();
        }
        total_size += kCrcSize + sizeof(kFooterSign);

        size_t offset = 0;

        // Copy header
        memcpy(out_buffer + offset, kHeaderSign, sizeof(kHeaderSign));
        offset += sizeof(kHeaderSign);

        // Copy message type
        memcpy(out_buffer + offset, &message_type, sizeof(int));
        offset += sizeof(int);

        // Copy payloads
        for (const auto& buf : buffers) {
            uint32_t buf_size = static_cast<uint32_t>(buf.size());
            memcpy(out_buffer + offset, &buf_size, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            memcpy(out_buffer + offset, buf.data(), buf_size);
            offset += buf_size;
        }

        // Compute and copy CRC (dummy CRC for now)
        uint32_t crc = 0; // Replace with actual CRC computation
        memcpy(out_buffer + offset, &crc, kCrcSize);
        offset += kCrcSize;

        // Copy footer
        memcpy(out_buffer + offset, kFooterSign, sizeof(kFooterSign));
        offset += sizeof(kFooterSign);

        out_size = static_cast<uint32_t>(total_size);
        return 0;
    }


    std::pair<char*, uint32_t> PipeElement::pack(int message_type, const char* buffer, uint32_t size) {
        uint32_t actual_size = sizeof(kHeaderSign) + sizeof(int) + size + kCrcSize + sizeof(kFooterSign);
        char* packed_buffer = new char[actual_size];
        uint32_t packed_size = 0;
        if (pack_to(message_type, buffer, size, packed_buffer, packed_size) != 0) {
            delete[] packed_buffer;
            return {nullptr, 0};
        }
        return {packed_buffer, packed_size};
    }

    std::pair<char*, uint32_t> PipeElement::pack(int message_type, int command_id, const char* buffer, uint32_t size) {
        uint32_t actual_size = sizeof(kHeaderSign) + sizeof(int) + sizeof(int) + size + kCrcSize + sizeof(kFooterSign);
        char* packed_buffer = new char[actual_size];
        uint32_t packed_size = 0;
        if (pack_to(message_type, command_id, buffer, size, packed_buffer, packed_size) != 0) {
            delete[] packed_buffer;
            return {nullptr, 0};
        }
        return {packed_buffer, packed_size};
    }

    int PipeElement::is_valid_packet(const char* buffer, uint32_t size) {
        if (size < sizeof(kHeaderSign) + kCrcSize + sizeof(kFooterSign)) {
            return -1; // Not enough data
        }

        // Check header
        if (memcmp(buffer, kHeaderSign, sizeof(kHeaderSign)) != 0) {
            return -2; // Invalid header
        }

        // Check footer
        if (memcmp(static_cast<const char*>(buffer) + size - sizeof(kFooterSign), kFooterSign, sizeof(kFooterSign)) != 0) {
            return -3; // Invalid footer
        }

        return kSuccessCode;
    }


    int PipeElement::unpack_to(const char* buffer, uint32_t size, int& message_type, char* out_buffer, uint32_t& out_size) {
        uint32_t offset = 0;

        // Extract message type
        memcpy(&message_type, static_cast<const char*>(buffer) + sizeof(kHeaderSign), sizeof(int));
        offset += sizeof(kHeaderSign) + sizeof(int);

        // Extract payload
        uint32_t payload_size = size - sizeof(kHeaderSign) - sizeof(int) - kCrcSize - sizeof(kFooterSign);
        memcpy(out_buffer, static_cast<const char*>(buffer) + offset, payload_size);
        out_size = payload_size;

        return 0;
    }

    int PipeElement::unpack(const char* buffer, uint32_t size, int& message_type, char*& out_buffer, uint32_t& out_size) {
        uint32_t offset = 0;

        // Extract message type
        memcpy(&message_type, static_cast<const char*>(buffer) + sizeof(kHeaderSign), sizeof(int));
        offset += sizeof(kHeaderSign) + sizeof(int);

        // Extract payload
        uint32_t payload_size = size - sizeof(kHeaderSign) - sizeof(int) - kCrcSize - sizeof(kFooterSign);
        out_buffer = new char[payload_size];
        memcpy(out_buffer, static_cast<const char*>(buffer) + offset, payload_size);
        out_size = payload_size;

        return 0;
    }

}