#include "pipe_server_ext.hpp"
#include "../core/pipehelpers.hpp"
#include "../payload/messages.hpp"
#include "../core/pipe_element.hpp"

namespace Injector {


PipeServer_EXT::PipeServer_EXT(std::wstring pipeName, Core::Logger &logger)
    : Core::PipeElement(INVALID_HANDLE_VALUE, logger), pipeName_(std::move(pipeName))
{
}

PipeServer_EXT::~PipeServer_EXT()
{
    if (hFile_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile_);
        hFile_ = INVALID_HANDLE_VALUE;
    }
}

int PipeServer_EXT::init()
{
    int retcode = -1;
    hFile_ = CreateNamedPipeW(pipeName_.c_str(), PIPE_ACCESS_DUPLEX,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                        1, Core::PipeElement::kBufferSize, Core::PipeElement::kBufferSize, 0, nullptr);
    if (hFile_ != INVALID_HANDLE_VALUE) {
        retcode = 0;
    }
    return retcode;
}

int PipeServer_EXT::wait_for_client(size_t timeout_ms)
{
    int retcode = -1;
    size_t deadline = GetTickCount64() + timeout_ms;
    while (GetTickCount64() < deadline) {
        if (hFile_ != INVALID_HANDLE_VALUE) {
            if (ConnectNamedPipe(hFile_, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
                retcode = 0;
                break;
            }
        }
        Sleep(Core::PipeElement::kSleepIntervalMs);
    }
    return retcode;
}

} // namespace Injector