// (c) Alexander 'xaitax' Hagenah
// Licensed under the MIT License. See LICENSE file in the project root for full license information.

#include "pipe_client_ext.hpp"
#include "../core/logger.hpp"
#include <windows.h>
#include <string>
#include "../injector/serializer.hpp"
#include "messages.hpp"
#include <iostream>
#include <stdexcept>
#include "../core/pipe_element.hpp"

namespace Payload {
    PipeClient_EXT::PipeClient_EXT(const std::wstring& pipeName, Core::Logger& logger)
        : Core::PipeElement(INVALID_HANDLE_VALUE, logger), pipeName_(pipeName) {
    }

    PipeClient_EXT::~PipeClient_EXT() {
        close();
    }

    bool PipeClient_EXT::connect() {
        if (hFile_ != INVALID_HANDLE_VALUE) {
            logger_.info(L"Pipe already connected.");
            return true;
        }
        logger_.info(L"Attempting to connect to pipe: " + pipeName_);
        hFile_ = CreateFileW(pipeName_.c_str(), GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        bool connected = hFile_ != INVALID_HANDLE_VALUE;
        logger_.info(connected ? L"Successfully connected to pipe." : L"Failed to connect to pipe.");
        return connected;
    }

    // bool PipeClient_EXT::disconnect() {
    //     if (hFile_ == INVALID_HANDLE_VALUE) {
    //         logger_.info(L"Pipe not connected.");
    //         return true;
    //     }

    //     FlushFileBuffers(hFile_);
    //     CloseHandle(hFile_);
    //     hFile_ = INVALID_HANDLE_VALUE;
    //     logger_.info(L"Successfully disconnected from pipe.");
    //     return true;
    // }


}