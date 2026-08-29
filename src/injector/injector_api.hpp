#pragma once

#include "../payload/messages.hpp"
#include "pipe_server.hpp"
#include <windows.h>


#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

namespace Injector {

class PipeServer_EXT : public PipeServer
{
    public:
        explicit PipeServer_EXT(const std::wstring& browserType) : PipeServer(browserType) {}
        int Send(Payload::Request& request, bool verbose);
        void SendMsg(const Payload::request_msg& request, bool verbose);
        void WaitResponse(const Payload::request_msg& request, Payload::response_msg& response, bool verbose, uint32_t timeout);
        void WaitAck(const Payload::request_msg& request, bool verbose, uint32_t timeout);
        void SendAck(const Payload::request_msg& request, bool verbose, uint32_t timeout);
        void WaitForClient(bool verbose, uint32_t timeout);
};


extern "C" {
    DLL_EXPORT int Process(Payload::Request& request);
    DLL_EXPORT int Encrypt(int browserId, std::wstring execPath, char* key, size_t keyLength, char* outEncryptedKey, size_t& outEncryptedKeyLength);
    DLL_EXPORT int Decrypt(int browserId, std::wstring execPath, char* encryptedKey, size_t encryptedKeyLength, char* outKey, size_t& outKeyLength);
}

} // namespace Injector
