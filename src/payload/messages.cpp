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

    // Couple maps to maintain the bidirectional mapping between type_index and integer IDs
    static std::unordered_map<std::type_index, int> s_type_idx_map;
    static std::unordered_map<int, std::type_index> s_idx_type_map;

    int register_type(std::type_index type_idx)
    {
        auto it = s_type_idx_map.find(type_idx);
        if (it != s_type_idx_map.end())
        {
            return it->second;
        }
        else
        {
            int idx = s_next_type_idx++;
            s_type_idx_map[type_idx] = idx;
            s_idx_type_map[idx] = type_idx;
            return idx;
        }
    }

    int get_id(std::type_index type_idx)
    {
        auto it = s_type_idx_map.find(type_idx);
        if (it != s_type_idx_map.end())
        {
            return it->second;
        }
        else
        {
            return -1; // Type not registered
        }
    }

    std::type_index get_type(int id)
    {
        auto it = s_idx_type_map.find(id);
        if (it != s_idx_type_map.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("Type ID not registered");
        }
    }

    std::pair<char*, uint32_t> pack(const char *data, uint32_t length)
    {
        // Pack the data with start and end signals
        uint32_t total_length = s_SOS_vec.size() + length + s_EOS_vec.size();
        char *buffer = new char[total_length];

        std::memcpy(buffer, s_SOS_vec.data(), s_SOS_vec.size());
        std::memcpy(buffer + s_SOS_vec.size(), data, length);
        std::memcpy(buffer + s_SOS_vec.size() + length, s_EOS_vec.data(), s_EOS_vec.size());

        return {buffer, total_length};
    }

    int pack_to(const char *data, uint32_t length, char *dst)
    {
        // Pack the data with start and end signals into the provided buffer
        std::memcpy(dst, s_SOS_vec.data(), s_SOS_vec.size());
        std::memcpy(dst + s_SOS_vec.size(), data, length);
        std::memcpy(dst + s_SOS_vec.size() + length, s_EOS_vec.data(), s_EOS_vec.size());
        return 0;
    }

    bool is_valid_msg(const char* buffer, uint32_t length)
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

    std::pair<char *, uint32_t> unpack(const char *buffer, uint32_t length)
    {
        if (!is_valid_msg(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }
        return { const_cast<char*>(buffer + s_SOS_vec.size()), length - s_SOS_vec.size() - s_EOS_vec.size() };
    }
    uint32_t unpack(const char *buffer, uint32_t length, char **payload, uint32_t &payload_length)
    {
        if (!is_valid_msg(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }

        // Allocate memory for the payload and copy the data
        payload_length = length - s_SOS_vec.size() - s_EOS_vec.size();
        *payload = new char[payload_length];
        std::memcpy(*payload, buffer + s_SOS_vec.size(), payload_length);
        return 0;
    }

    uint32_t unpack_to(const char *buffer, uint32_t length, char *dst, uint32_t &payload_length)
    {
        if (!is_valid_msg(buffer, length))
        {
            throw std::runtime_error("Invalid message format");
        }

        payload_length = length - s_SOS_vec.size() - s_EOS_vec.size();
        std::memcpy(dst, buffer + s_SOS_vec.size(), payload_length);

        return 0;
    }
}
