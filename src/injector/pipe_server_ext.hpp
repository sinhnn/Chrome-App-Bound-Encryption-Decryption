#pragma once

#include "pipe_server.hpp"
#include "../core/common.hpp"
#include "../core/console.hpp"
#include "../sys/internal_api.hpp"
#include "browser_discovery.hpp"
#include "browser_terminator.hpp"
#include "process_manager.hpp"
// #include "pipe_server.hpp"
#include "../core/pipe_element.hpp"
#include "injector.hpp"
#include <iostream>
#include "../payload/messages.hpp"
#include "serializer.hpp"

namespace Injector
{
    class PipeServer_EXT : public Core::PipeElement
    {
    public:
        explicit PipeServer_EXT(std::wstring pipeName, Core::Logger &logger);
        ~PipeServer_EXT();
        int init();
        int wait_for_client(size_t timeout_ms);

    private:
        std::wstring pipeName_;
    };

} // namespace Injector