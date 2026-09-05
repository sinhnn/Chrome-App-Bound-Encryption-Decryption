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
    struct ISerializable
    {
        virtual ~ISerializable() = default;
        virtual int serialize_to(char* out_buffer, uint32_t& out_size) const = 0;
        virtual int deserialize_from(const char* in_buffer, uint32_t in_size) = 0;
        virtual uint32_t get_size() const = 0;
    };

    struct Buffer {
        uint32_t size;
        void* ptr;
    };

    // [number_item][offset0][offset1]...[offset_n-1][??offset_n][item_0][item_1]...[item_n-1]
    class MemoryLayout {
        public:
        MemoryLayout(char* ptr, uint32_t size) : ptr_(ptr), size_(size) {
            cached_number_items_ =  *reinterpret_cast<const uint32_t*>(ptr);
            if (cached_number_items_ == 0) {
                throw std::invalid_argument("MemoryLayout must contain at least one item");
            }
        }

        MemoryLayout (const Buffer& buffer) : MemoryLayout(static_cast<char*>(buffer.ptr), buffer.size) {}
        MemoryLayout (const Buffer* buffer) : MemoryLayout(static_cast<char*>(buffer->ptr), buffer->size) {}

        uint32_t get_number_items() const {
            return cached_number_items_;
        }

        uint32_t get_item_offset(uint32_t index) const {
            if (index >= cached_number_items_) {
                throw std::out_of_range("Index out of range");
            }
            return *reinterpret_cast<const uint32_t*>(ptr_ + sizeof(uint32_t) + index * sizeof(uint32_t));
        }

        uint32_t get_item_size(uint32_t index) const {
            if (index >= cached_number_items_) {
                throw std::out_of_range("Index out of range");
            }

            return index == cached_number_items_ - 1 ? size_ - get_item_offset(index) : get_item_offset(index + 1) - get_item_offset(index);
        }

        Buffer get_item(uint32_t index) const {
            Buffer buffer;
            buffer.size = get_item_size(index);
            buffer.ptr = ptr_ + get_item_offset(index);
            return buffer;
        }

        char* get_ptr_item(uint32_t index) const {
            if (index >= cached_number_items_) {
                throw std::out_of_range("Index out of range");
            }
            return ptr_ + get_item_offset(index);
        }

        private:
            char* ptr_;
            uint32_t size_;
            // cached data for item offsets
            uint32_t cached_number_items_ {0};
    };

    // struct Request__struct
    // {
    //     int request_id;
    //     int command_id;
    //     std::vector<Buffer> args;
    // };

    // struct Response__struct
    // {
    //     int request_id;
    //     int status_code;
    //     std::vector<Buffer> args;
    // };

    // class Request__block {
    //     public:
    //         Request__block(char* ptr, uint32_t size) : ptr(ptr), size(size) {}

    //         int get_request_id() const {
    //             return *reinterpret_cast<const int*>(ptr);
    //         }

    //         int get_command_id() const {
    //             return *reinterpret_cast<const int*>(ptr + sizeof(int));
    //         }

    //         const char* get_args_ptr() const {
    //             return ptr + 2 * sizeof(int);
    //         }

    //         std::vector<Buffer> get_args() const {
    //             std::vector<Buffer> args;
    //             const char* args_ptr = get_args_ptr();
    //             uint32_t remaining_size = size - 2 * sizeof(int);
    //             while (remaining_size > 0) {
    //                 uint32_t arg_size = *reinterpret_cast<const uint32_t*>(args_ptr);
    //                 args_ptr += sizeof(uint32_t);
    //                 args.push_back({arg_size, const_cast<char*>(args_ptr)});
    //                 args_ptr += arg_size;
    //                 remaining_size -= sizeof(uint32_t) + arg_size;
    //             }
    //             return args;
    //         }

    //         int get_size() const { return size; }
    //         char* get_ptr() const { return ptr; }

    //     private:
    //         char* ptr;
    //         uint32_t size;
    // };

    // class Response {
    // public:
    //     Response(const char* buffer, uint32_t size) {
    //         // Implement deserialization logic for Response
    //         this->buffer_ = buffer;
    //         this->size_ = size;
    //     }

    //     int get_command_id() const {
    //         return *reinterpret_cast<const int*>(buffer_);
    //     }

    //     std::vector<std::vector<char>> get_args() {
    //         if (initialized_args_) {
    //             return args_;
    //         }
    //         uint32_t offset = sizeof(int); // Skip command_id
    //         while (offset < size_) {
    //             uint32_t arg_size = *reinterpret_cast<const uint32_t*>(buffer_ + offset);
    //             offset += sizeof(uint32_t);
    //             std::vector<char> arg(buffer_ + offset, buffer_ + offset + arg_size);
    //             args_.push_back(arg);
    //             offset += arg_size;
    //         }
    //         initialized_args_ = true;
    //         return args_;
    //     }
    // protected:
    //     uint32_t size_;
    //     const char* buffer_;
    //     std::vector<std::vector<char>> args_;
    //     bool initialized_args_ = false;
    // };


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
        int write(int message_type, const char* buffer, uint32_t size);
        int write(int message_type, std::vector<std::vector<char>> buffers);
        int write(int message_type, const ISerializable& serializable);
        int write(int message_type, std::vector<ISerializable*> serializables);
        int write(int message_type, std::vector<Buffer> buffers);
        int write(int message_type, std::vector<Buffer*> buffers);

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
        static int pack_to(int message_type, const char* buffer, uint32_t size, char* out_buffer, uint32_t& out_size);
        static int pack_to(int message_type, std::vector<std::vector<char>> buffers, char* out_buffer, uint32_t& out_size);
        static int pack_to(int message_type, const ISerializable& serializable, char* out_buffer, uint32_t& out_size);
        static std::pair<char*, uint32_t> pack(int message_type, const char* buffer, uint32_t size);

        static int is_valid_packet(const char* buffer, uint32_t size);

        static int unpack_to(const char* buffer, uint32_t size, int& message_type, char* out_buffer, uint32_t& out_size);
        static int unpack(const char* buffer, uint32_t size, int& message_type, char*& out_buffer, uint32_t& out_size);

        class _BaseROR
        {
        public:
            _BaseROR(char* buffer, int size) : buffer_(buffer), size_(size) {}

            int get_command_id_size() const {
                return *reinterpret_cast<int*>(buffer_);
            }

            int get_command_id() const {
                return *reinterpret_cast<int*>(buffer_ + get_command_id_size());
            }

            char* get_payload() const {
                return buffer_ + sizeof(int) + get_command_id_size();
            }

            int get_payload_size() const {
                return size_ - sizeof(int) - get_command_id_size();
            }

            std::vector<std::vector<char>> get_buffers() {
                if (!buffers_initialized_) {
                    size_t offset = sizeof(int) + get_command_id_size();
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
