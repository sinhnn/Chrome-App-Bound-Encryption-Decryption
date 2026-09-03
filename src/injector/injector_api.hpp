#pragma once

#include "../payload/messages.hpp"
#include "pipe_server.hpp"
#include <windows.h>


#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

namespace Injector {
extern "C" {
    // DLL_EXPORT int Process(Payload::Request& request);
    DLL_EXPORT int Encrypt(std::wstring execPath, char* key, uint32_t keyLength, char* outEncryptedKey, uint32_t& outEncryptedKeyLength);
    DLL_EXPORT int Decrypt(std::wstring execPath, char* encryptedKey, uint32_t encryptedKeyLength, char* outKey, uint32_t& outKeyLength);
}

} // namespace Injector
