// (c) Alexander 'xaitax' Hagenah
// Licensed under the MIT License. See LICENSE file in the project root for full license information.

#pragma once

#include "../core/common.hpp"
#include "messages.hpp"
#include "../core/logger.hpp"
#include "../core/pipe_element.hpp"
#include "pipe_client.hpp"
#include <string>

namespace Payload {

    class PipeClient_EXT : public Core::PipeElement {
    public:
        explicit PipeClient_EXT(const std::wstring& pipeName, Core::Logger& logger);
        ~PipeClient_EXT();
        bool connect();
        // bool disconnect();
    private:
        std::wstring pipeName_;
    };

}
