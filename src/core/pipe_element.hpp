#pragma once

#include <Windows.h>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include "logger.hpp"
#include "constants.hpp"
#include <vector>

namespace Core
{
    struct Packet {
        int message_type;
        uint32_t size;
        const char* buffer;
    };

    class PipeElement
    {
    public:
        static constexpr int kSuccessCode = 0;
        static constexpr int kInvalidCCommandId = -1;
        static constexpr int kSleepIntervalMs = 50;
        static constexpr int kBufferSize = 4096;
        static constexpr int kMessageType_Unknown = 0;
        static constexpr int kMessageType_Text = 1;
        static constexpr int kMessageType_Binary = 2;
        static constexpr int kMessageType_CloseTunnel = 3;
        static constexpr int kMessageType_Request = 4;
        static constexpr int kMessageType_Response = 5;
        // signal end of stream
        static constexpr char kCloseTunnelMsg[] = {'\0'};

        PipeElement(HANDLE hFile, Core::Logger& logger);
        virtual ~PipeElement();
        int close();
        bool is_valid() const;
        // int write(char* buffer, size_t size);
        int write(int message_type, const char* buffer, uint32_t size);
        int write(int message_type, int command_id, const char* buffer, uint32_t size);
        int write(int message_type, int command_id, std::vector<std::vector<char>> buffers);

        int send_text(std::wstring message);
        int register_handler(int message_type, std::function<void(const char* buffer, uint32_t size)> handler);
        int unregister_handler(int message_type);
        int read_utils(std::function<bool(int msg_type, const char* buffer, uint32_t size)> delegate, uint32_t timeout_ms);

    protected:
        std::unordered_map<int, std::function<void(const char* buffer, uint32_t size)>> handlers_;
        HANDLE hFile_;
        Core::Logger& logger_;
        int write_ (const char* buffer, uint32_t size, DWORD& bytesWritten);

    public:
        // Packet format:
        // [HeaderSign][Payload][CRC][FooterSign]

        // Message format:
        // [MessageType][Payload]

        // Request | Response format:
        // [MessageType][CommandID][Payload]

        // static int pack_to(const char* buffer, size_t size, char* out_buffer, size_t& out_size);
        // static std::pair<char*, size_t> pack(const char* buffer, size_t size);
        static int pack_to(int message_type, const char* buffer, uint32_t size, char* out_buffer, uint32_t& out_size);
        // Used to packe request and reponse with separate command ID, typically for correlating requests and responses
        static int pack_to(int message_type, int command_id, const char* buffer, uint32_t size, char* out_buffer, uint32_t& out_size);
        // Used to pack multiple buffers into a single message with a command ID
        static int pack_to(int message_type, int command_id, std::vector<std::vector<char>> buffers, char* out_buffer, uint32_t& out_size);
        static int pack_to(int message_type, std::vector<std::vector<char>> buffers, char* out_buffer, uint32_t& out_size);

        static std::pair<char*, uint32_t> pack(int message_type, const char* buffer, uint32_t size);
        // Used to pack request and response with separate command ID, typically for correlating requests and responses
        static std::pair<char*, uint32_t> pack(int message_type, int command_id, const char* buffer, uint32_t size);

        static int is_valid_packet(const char* buffer, uint32_t size);

        // static int unpack_to(const char* buffer, uint32_t size, char* out_buffer, uint32_t& out_size);
        // static int unpack(const char* buffer, uint32_t size, char*& out_buffer, uint32_t& out_size);

        static int unpack_to(const char* buffer, uint32_t size, int& message_type, char* out_buffer, uint32_t& out_size);
        // static int unpack_to(const char* buffer, uint32_t size, int& message_type, int& command_id, char* out_buffer, uint32_t& out_size);
        static int unpack(const char* buffer, uint32_t size, int& message_type, char*& out_buffer, uint32_t& out_size);
        // static int unpack(const char* buffer, uint32_t size, int& message_type, int& command_id,char*& out_buffer, uint32_t& out_size);

        class _BaseROR
        {
        public:
            _BaseROR(char* buffer, int size) : buffer_(buffer), size_(size) {}


            int get_command_id() const {
                return *reinterpret_cast<int*>(buffer_);
            }

            char* get_payload() const {
                return buffer_ + sizeof(int);
            }

            int get_payload_size() const {
                return size_ - sizeof(int);
            }

            std::vector<std::vector<char>> get_buffers() {
                if (!buffers_initialized_) {
                    // Initialize buffers_ if it's not initialized
                    size_t offset = sizeof(int);
                    while (offset < static_cast<size_t>(size_)) {
                        if (offset + sizeof(uint32_t) > static_cast<size_t>(size_)) {
                            break; // Invalid buffer size
                        }
                        uint32_t buf_size = *reinterpret_cast<uint32_t*>(buffer_ + offset);
                        offset += sizeof(uint32_t);
                        if (offset + buf_size > static_cast<size_t>(size_)) {
                            break; // Invalid buffer size
                        }
                        std::vector<char> buf(buffer_ + offset, buffer_ + offset + buf_size);
                        buffers_.push_back(buf);
                        offset += buf_size;
                    }
                    buffers_initialized_ = true;
                }
                return buffers_;
            }

        protected:
            char* buffer_;
            int size_;
            std::vector<std::vector<char>> buffers_;
            bool buffers_initialized_ {false};
        };

        class Request : public _BaseROR
        {
        public:
            Request(char* buffer, int size) : _BaseROR(buffer, size) {}
        };

        class Response : public _BaseROR
        {
        public:
            Response(char* buffer, int size) : _BaseROR(buffer, size) {}
        };


    };

}