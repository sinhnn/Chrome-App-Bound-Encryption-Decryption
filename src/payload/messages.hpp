#pragma once

#include <string>
#include <vector>
#include <map>
#include <typeindex>
#include <typeinfo>
#include <cstring>
#include <typeinfo>

#define MAX_MESSAGE_SIZE 4096
namespace Payload
{

    enum class Command
    {
        NONE = 0,
        DECRYPT_APP_BOUND_ENCRYPTED_KEY,
        ENCRYPT_APP_BOUND_KEY,
    };

    enum class MessageType
    {
        NONE,
        REQUEST,
        RESPONSE,
        LOG,
    };

    extern const std::vector<uint8_t> s_SOS_vec;
    extern const std::vector<uint8_t> s_EOS_vec;
    extern const size_t kMaxMessageSize;

    uint32_t calculate_crc32(const char *data, size_t length);

    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;
        virtual size_t size() const = 0;
        virtual int serialize_to(char *dst, size_t &written) const = 0;
        virtual std::pair<char *, size_t> serialize() const {
            size_t total_length = size();
            char *buffer = new char[total_length];
            size_t written = 0;
            serialize_to(buffer, written);
            return {buffer, total_length};
        }
    };

    struct PacketHeader {
        size_t payload_length;
    };

    class Packet
    {
    public:
        Packet(size_t payload_length, char *payload)
            : payload_length(payload_length), payload(payload) {}

        size_t size() const
        {
            return s_SOS_vec.size() + sizeof(payload_length) + payload_length + s_EOS_vec.size();
        }

        size_t payload_size() const
        {
            return payload_length;
        }

        char* get_payload() const
        {
            return payload;
        }

        int serialize_to(char *dst, size_t &written) const
        {
            std::memcpy(dst, s_SOS_vec.data(), s_SOS_vec.size());
            std::memcpy(dst + s_SOS_vec.size(), &payload_length, sizeof(payload_length));
            std::memcpy(dst + s_SOS_vec.size() + sizeof(payload_length), payload, payload_length);
            std::memcpy(dst + s_SOS_vec.size() + sizeof(payload_length) + payload_length, s_EOS_vec.data(), s_EOS_vec.size());
            written = size();
            return 0; // Return 0 for success
        }

    private:
        size_t payload_length;
        char *payload;

    public:
        static Packet* from_raw(const char *buffer, size_t length) {
            Packet* pkt = nullptr;
            if (length >= s_SOS_vec.size() + sizeof(size_t) + s_EOS_vec.size()) {
                pkt = reinterpret_cast<Packet*>(const_cast<char*>(buffer + s_SOS_vec.size()));
            }
            return pkt;
        }
        static size_t calculate_pack_size(size_t payload_length);
        static std::pair<char *, size_t> pack(const char *data, size_t length);
        static int pack_to(const char *data, size_t length, char *dst);
        static bool is_valid_packet(const char *buffer, size_t length);
        static std::pair<char *, size_t> unpack(const char *buffer, size_t length);
        static size_t unpack(const char *buffer, size_t length, char **payload, size_t &payload_length);
        static size_t unpack_to(const char *buffer, size_t length, char *dst, size_t &payload_length);
    };

    struct message : public ISerializable
    {
        uint32_t msg_type;
        size_t payload_length;
        char *payload;

        message() : msg_type(0), payload_length(0), payload(nullptr) {}
        message(uint32_t msg_type, size_t payload_length, char *payload)
        {
            this->msg_type = msg_type;
            this->payload_length = payload_length;
            this->payload = new char[payload_length];
            std::memcpy(this->payload, payload, payload_length);
        }
        ~message()
        {
            delete[] payload;
        }

        size_t size() const override
        {
            return sizeof(msg_type) + sizeof(payload_length) + payload_length;
        }

        int serialize_to(char *dst, size_t &written) const override
        {
            std::memcpy(dst, &msg_type, sizeof(msg_type));
            std::memcpy(dst + sizeof(msg_type), &payload_length, sizeof(payload_length));
            std::memcpy(dst + sizeof(msg_type) + sizeof(payload_length), payload, payload_length);
            written = size();
            return 0; // Return 0 for success
        }
    };


    struct request_msg : public message
    {
        request_msg() : message(static_cast<uint32_t>(MessageType::REQUEST), 0, nullptr) {}
        request_msg(size_t payload_length, char *payload)
            : message(static_cast<uint32_t>(MessageType::REQUEST), payload_length, payload) {}
        ~request_msg()
        {
            delete[] payload;
        }
    };

    struct log_message : public message
    {
        log_message() : message(static_cast<uint32_t>(MessageType::LOG), 0, nullptr) {}
        log_message(std::wstring message)
            : message(
                static_cast<uint32_t>(MessageType::LOG),
                message.size() * sizeof(wchar_t),
                reinterpret_cast<char *>(const_cast<wchar_t *>(message.data()))
            ) {}
        ~log_message()
        {
            delete[] payload;
        }
    };

    class response_msg : public message
    {
    public:
        response_msg() : message(static_cast<uint32_t>(MessageType::RESPONSE), 0, nullptr) {}
        response_msg(int status_code, size_t payload_length, char *payload)
            : status_code_(status_code)
            , message(static_cast<uint32_t>(MessageType::RESPONSE), payload_length, payload) {}

        size_t size() const override
        {
            return sizeof(status_code_) + message::size();
        }

        int serialize_to(char *dst, size_t &written) const override
        {
            std::memcpy(dst, &status_code_, sizeof(status_code_));
            size_t written_message = 0;
            message::serialize_to(dst + sizeof(status_code_), written_message);
            written = size();
            return 0; // Return 0 for success
        }

        ~response_msg()
        {
            delete[] payload;
        }
    private:
        int status_code_ {-1};
    };

    struct browser_request_msg : public ISerializable
    {
    public:
        uint32_t command_id;
        uint32_t browser_exec_path_length;
        char *browser_exec_path;
        uint32_t content_size;
        char *content;

        browser_request_msg() : command_id(0), browser_exec_path_length(0), browser_exec_path(nullptr), content_size(0), content(nullptr) {}
        browser_request_msg(uint32_t browser_exec_path_length, char *browser_exec_path, uint32_t content_size, char *content)
        {
            this->browser_exec_path_length = browser_exec_path_length;
            this->browser_exec_path = new char[browser_exec_path_length];
            std::memcpy(this->browser_exec_path, browser_exec_path, browser_exec_path_length);
            this->content_size = content_size;
            this->content = new char[content_size];
            std::memcpy(this->content, content, content_size);
        }

        size_t size() const override
        {
            return sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length + sizeof(content_size) + content_size;
        }

        int serialize_to(char *dst, size_t &written) const override
        {
            std::memcpy(dst, &command_id, sizeof(command_id));
            std::memcpy(dst + sizeof(command_id), &browser_exec_path_length, sizeof(browser_exec_path_length));
            std::memcpy(dst + sizeof(command_id) + sizeof(browser_exec_path_length), browser_exec_path, browser_exec_path_length);
            std::memcpy(dst + sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length, &content_size, sizeof(content_size));
            std::memcpy(dst + sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length + sizeof(content_size), content, content_size);
            written = size();
            return 0; // Return 0 for success
        }

        static browser_request_msg *create(
            uint32_t command_id,
            uint32_t browser_exec_path_length,
            char *browser_exec_path,
            uint32_t content_size,
            char *content)
        {
            size_t total_size = sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length + sizeof(content_size) + content_size;
            void *buffer = std::malloc(total_size);
            if (!buffer)
                return nullptr;
            // Fullfill data into the allocated buffer with continuous memory layout
            std::memcpy(buffer, &command_id, sizeof(command_id));
            std::memcpy(static_cast<char *>(buffer) + sizeof(command_id), &browser_exec_path_length, sizeof(browser_exec_path_length));
            std::memcpy(static_cast<char *>(buffer) + sizeof(command_id) + sizeof(browser_exec_path_length), browser_exec_path, browser_exec_path_length);
            std::memcpy(static_cast<char *>(buffer) + sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length, &content_size, sizeof(content_size));
            std::memcpy(static_cast<char *>(buffer) + sizeof(command_id) + sizeof(browser_exec_path_length) + browser_exec_path_length + sizeof(content_size), content, content_size);
            return static_cast<browser_request_msg *>(buffer);
        }
    };

    class ack_msg : public ISerializable
    {
    public:
        ack_msg() : ack_code(0) {}

        uint32_t ack_code;

        size_t size() const override
        {
            return sizeof(ack_code);
        }

        int serialize_to(char *dst, size_t &written) const override
        {
            std::memcpy(dst, &ack_code, sizeof(ack_code));
            written = size();
            return 0; // Return 0 for success
        }
    };

    class Request
    {
    public:
        Request(const request_msg &req) : request(req)
        {
        }

        Request() : request(request_msg())
        {
        }

        const request_msg &request;
        response_msg response;
    };

} // namespace Payload
