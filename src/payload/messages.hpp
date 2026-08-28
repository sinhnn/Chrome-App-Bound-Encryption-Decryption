#pragma once

#include <string>
#include <vector>
#include <map>
#include <typeindex>
#include <typeinfo>
#include <cstring>
#include <typeinfo>


namespace Payload {

    enum class Command {
        NONE,
        DECRYPT_APP_BOUND_ENCRYPTED_KEY,
        ENCRYPT_APP_BOUND_KEY,
    };

    extern const std::vector<uint8_t> s_SOS_vec;
    extern const std::vector<uint8_t> s_EOS_vec;

    uint32_t calculate_crc32(const char* data, size_t length);

    class ISerializable {
    public:
        virtual ~ISerializable() = default;
        virtual std::pair<char*, size_t> Serialize() const = 0;
        virtual size_t size() const = 0;
        virtual int SerializeTo(char* dst, size_t& written) const = 0;
        // virtual void Deserialize(const std::vector<uint8_t>& data) = 0;
    };

    struct Message {
        uint32_t type;
        size_t length;
        char* payload;

        size_t size() const {
            return sizeof(type) + sizeof(length) + length;
        }

        char* Serialize() const {
            char* buffer = new char[sizeof(type) + sizeof(length) + length];
            std::memcpy(buffer, &type, sizeof(type));
            std::memcpy(buffer + sizeof(type), &length, sizeof(length));
            std::memcpy(buffer + sizeof(type) + sizeof(length), payload, length);
            return buffer;
        }

        int SerializeTo(char* dst, size_t& written) const {
            std::memcpy(dst, &type, sizeof(type));
            std::memcpy(dst + sizeof(type), &length, sizeof(length));
            std::memcpy(dst + sizeof(type) + sizeof(length), payload, length);
            written = size();
            return 0; // Return 0 for success
        }

        static Message Deserialize(const char* data) {
            Message msg;
            std::memcpy(&msg.type, data, sizeof(msg.type));
            std::memcpy(&msg.length, data + sizeof(msg.type), sizeof(msg.length));
            msg.payload = new char[msg.length];
            std::memcpy(msg.payload, data + sizeof(msg.type) + sizeof(msg.length), msg.length);
            return msg;
        }
    };

    class request_msg : public ISerializable {
        public:
        // Command identifier for the request, which leads to how to serialize/deserialize the payload
        uint32_t command_id;
        // Payload size
        size_t payload_length;
        // Payload data
        char* payload;

        std::pair<char*, size_t> Serialize() const override {
            size_t total_length = sizeof(command_id) + sizeof(payload_length) + payload_length;
            char* buffer = new char[total_length];
            std::memcpy(buffer, &command_id, sizeof(command_id));
            std::memcpy(buffer + sizeof(command_id), &payload_length, sizeof(payload_length));
            std::memcpy(buffer + sizeof(command_id) + sizeof(payload_length), payload, payload_length);
            return { buffer, total_length };
        }

        size_t size() const override {
            return sizeof(command_id) + sizeof(payload_length) + payload_length;
        }

        int SerializeTo(char* dst, size_t& written) const override {
            std::memcpy(dst, &command_id, sizeof(command_id));
            std::memcpy(dst + sizeof(command_id), &payload_length, sizeof(payload_length));
            std::memcpy(dst + sizeof(command_id) + sizeof(payload_length), payload, payload_length);
            written = size();
            return 0; // Return 0 for success
        }
    };

    class response_msg : public ISerializable {
        public:
        response_msg() : status_code(0), payload_length(0), payload(nullptr) {}

        uint32_t status_code;
        size_t payload_length;
        char* payload;

        std::pair<char*, size_t> Serialize() const override {
            size_t total_length = sizeof(status_code) + sizeof(payload_length) + payload_length;
            char* buffer = new char[total_length];
            std::memcpy(buffer, &status_code, sizeof(status_code));
            std::memcpy(buffer + sizeof(status_code), &payload_length, sizeof(payload_length));
            std::memcpy(buffer + sizeof(status_code) + sizeof(payload_length), payload, payload_length);
            return { buffer, total_length };
        }

        size_t size() const override {
            return sizeof(status_code) + sizeof(payload_length) + payload_length;
        }

        int SerializeTo(char* dst, size_t& written) const override {
            std::memcpy(dst, &status_code, sizeof(status_code));
            std::memcpy(dst + sizeof(status_code), &payload_length, sizeof(payload_length));
            std::memcpy(dst + sizeof(status_code) + sizeof(payload_length), payload, payload_length);
            written = size();
            return 0; // Return 0 for success
        }
    };

    class ack_msg : public ISerializable {
        public:
        ack_msg() : ack_code(0) {}

        uint32_t ack_code;

        std::pair<char*, size_t> Serialize() const override {
            char* buffer = new char[sizeof(ack_code)];
            std::memcpy(buffer, &ack_code, sizeof(ack_code));
            return { buffer, sizeof(ack_code) };
        }

        size_t size() const override {
            return sizeof(ack_code);
        }

        int SerializeTo(char* dst, size_t& written) const override {
            std::memcpy(dst, &ack_code, sizeof(ack_code));
            written = size();
            return 0; // Return 0 for success
        }
    };

    class Request {
        public:
        Request(const request_msg& req) : request(req)
        {
        }

        Request() : request(request_msg())
        {
        }

        const request_msg& request;
        response_msg response;
    };

    size_t calculate_pack_size(size_t payload_length);
    std::pair<char*, size_t> pack(const char* data, size_t length);
    int pack_to(const char* data, size_t length, char* dst);

    bool is_valid_msg(const char* buffer, size_t length);

    std::pair<char*, size_t> unpack(const char* buffer, size_t length);
    size_t unpack(const char* buffer, size_t length, char** payload, size_t& payload_length);
    size_t unpack_to(const char* buffer, size_t length, char* dst, size_t& payload_length);

} // namespace Payload
