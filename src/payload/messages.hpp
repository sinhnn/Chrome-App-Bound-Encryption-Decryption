#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <typeindex>
#include <typeinfo>


namespace Payload {
    class ISerializable {
    public:
        virtual ~ISerializable() = default;
        virtual std::pair<char*, uint32_t> Serialize() const = 0;
        virtual void Deserialize(const std::vector<uint8_t>& data) = 0;
    };

    struct Message {
        uint32_t type;
        uint32_t length;
        char* payload;

        char* Serialize() const {
            // Serialize the message to a string representation
            // This is a placeholder implementation; actual serialization logic will depend on the message structure
            char* buffer = new char[sizeof(type) + sizeof(length) + length];
            std::memcpy(buffer, &type, sizeof(type));
            std::memcpy(buffer + sizeof(type), &length, sizeof(length));
            std::memcpy(buffer + sizeof(type) + sizeof(length), payload, length);
            return buffer;
        }

        static Message Deserialize(const char* data) {
            // Deserialize the string representation back to a Message object
            // This is a placeholder implementation; actual deserialization logic will depend on the message structure
            Message msg;
            std::memcpy(&msg.type, data, sizeof(msg.type));
            std::memcpy(&msg.length, data + sizeof(msg.type), sizeof(msg.length));
            msg.payload = new char[msg.length];
            std::memcpy(msg.payload, data + sizeof(msg.type) + sizeof(msg.length), msg.length);
            return msg;
        }
    };

    struct decrypt_request_msg {
        std::wstring browserPath;
    };

    struct app_bound_encrypted_key__decrypt_request_msg : public ISerializable {
    };

    struct app_bound_encrypted_key__decrypt_response_msg : public ISerializable {
    };

    struct app_bound_encyrypted_key__encrypt_request_msg : public ISerializable {
    };

    struct app_bound_encyrypted_key__encrypt_response_msg : public ISerializable {
    };

    // static std::map<std::type_index, uint32_t> id_to_msg_type;

}