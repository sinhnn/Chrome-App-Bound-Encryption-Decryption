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
        void SendMessage(const Payload::request_msg& request, bool verbose);
        void WaitResponse(const Payload::request_msg& request, Payload::response_msg& response, bool verbose, uint32_t timeout);
        void WaitAck(const Payload::request_msg& request, bool verbose, uint32_t timeout);
        void SendAck(const Payload::request_msg& request, bool verbose, uint32_t timeout);
};


extern "C" {
    DLL_EXPORT int Process(Payload::Request& request);
}

} // namespace Injector
