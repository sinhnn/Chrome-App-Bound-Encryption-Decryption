#include "messages.hpp"
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <stdexcept>


namespace Payload {
    // TODO: must locks if support multi-threading
    static int s_next_type_idx = 0;
    std::string s_SOS = "__SOS__";
    std::string s_EOS = "__EOS__";
    const std::vector<uint8_t> s_SOS_vec(s_SOS.begin(), s_SOS.end());
    const std::vector<uint8_t> s_EOS_vec(s_EOS.begin(), s_EOS.end());
    const size_t kMaxMessageSize = 4096; // 4096 + s_EOS_vec.size() + s_SOS_vec.size()

    static uint32_t crc32_table[256];
    static int table_initialized = 0;
    uint32_t calculate_crc32(const char *data, size_t length)
    {
        if (!table_initialized) {
            for (uint32_t i = 0; i < 256; ++i) {
                uint32_t crc = i;
                for (uint32_t j = 0; j < 8; ++j) {
                    if (crc & 1)
                        crc = (crc >> 1) ^ 0xEDB88320;
                    else
                        crc >>= 1;
                }
                crc32_table[i] = crc;
            }
            table_initialized = 1;
        }

        uint32_t crc = 0xFFFFFFFF;
        for (uint32_t i = 0; i < length; ++i) {
            uint8_t byte = static_cast<uint8_t>(data[i]);
            crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
        }
        return crc ^ 0xFFFFFFFF;
    }

    size_t Packet::calculate_pack_size(size_t payload_length)
    {
        return s_SOS_vec.size() + payload_length + s_EOS_vec.size();
    }

    std::pair<char *, size_t> Packet::pack(const char *data, size_t length)
    {
        // Pack the data with start and end signals
        size_t total_length = s_SOS_vec.size() + length + s_EOS_vec.size();
        char *buffer = new char[total_length];

        std::memcpy(buffer, s_SOS_vec.data(), s_SOS_vec.size());
        std::memcpy(buffer + s_SOS_vec.size(), data, length);
        std::memcpy(buffer + s_SOS_vec.size() + length, s_EOS_vec.data(), s_EOS_vec.size());

        return {buffer, total_length};
    }

    int Packet::pack_to(const char *data, size_t length, char *dst)
    {
        // Pack the data with start and end signals into the provided buffer
        std::memcpy(dst, s_SOS_vec.data(), s_SOS_vec.size());
        std::memcpy(dst + s_SOS_vec.size(), data, length);
        std::memcpy(dst + s_SOS_vec.size() + length, s_EOS_vec.data(), s_EOS_vec.size());
        return 0;
    }

    bool Packet::is_valid_packet(const char* buffer, size_t length)
    {
        if (length < s_SOS_vec.size() + s_EOS_vec.size())
        {
            return false; // Not enough length for start and end signals
        }

        // Check start signal
        if (!std::equal(s_SOS_vec.begin(), s_SOS_vec.end(), buffer))
        {
            return false; // Start signal does not match
        }

        // Check end signal
        if (!std::equal(s_EOS_vec.rbegin(), s_EOS_vec.rend(), buffer + length - s_EOS_vec.size()))
        {
            return false; // End signal does not match
        }

        return true; // Valid message
    }

    std::pair<char *, size_t> Packet::unpack(const char *buffer, size_t length)
    {
        if (!is_valid_packet(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }
        return { const_cast<char*>(buffer + s_SOS_vec.size()), length - s_SOS_vec.size() - s_EOS_vec.size() };
    }
    size_t Packet::unpack(const char *buffer, size_t length, char **payload, size_t &payload_length)
    {
        if (!is_valid_packet(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }

        // Allocate memory for the payload and copy the data
        payload_length = length - s_SOS_vec.size() - s_EOS_vec.size();
        *payload = new char[payload_length];
        std::memcpy(*payload, buffer + s_SOS_vec.size(), payload_length);
        return 0;
    }

    size_t Packet::unpack_to(const char *buffer, size_t length, char *dst, size_t &payload_length)
    {
        if (!is_valid_packet(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }

        payload_length = length - s_SOS_vec.size() - s_EOS_vec.size();
        std::memcpy(dst, buffer + s_SOS_vec.size(), payload_length);

        return 0;
    }
}
