#pragma once

#include "../payload/messages.hpp"
#include "pipe_server.hpp"

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


int Process(Payload::Request& request);

} // namespace Injector
