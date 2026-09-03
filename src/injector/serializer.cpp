#include <vector>
#include <string>
#include <windows.h>
#include "../core/common.hpp"
#include "browser_discovery.hpp"
#include "serializer.hpp"
#include <iostream>
#pragma comment(lib, "Version.lib")

namespace Injector {

const std::vector<std::wstring> StandardProperties = {
    L"CompanyName",
    L"FileDescription",
    L"FileVersion",
    L"InternalName",
    L"LegalCopyright",
    L"LegalTrademarks",
    L"OriginalFilename",
    L"ProductName",
    L"ProductVersion",
    L"Comments",
    L"PrivateBuild",
    L"SpecialBuild"
};


BrowserInfo* CreateBrowserInfoFromExecFilePath(const std::wstring& execFilePath) {
    BrowserInfo* browser = new BrowserInfo();
    browser->fullPath = execFilePath;
    // Get file status or other attributes if needed
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExW(execFilePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        throw std::runtime_error("Failed to get file attributes for " + Core::ToUtf8(execFilePath));
    }

    browser->type = std::wstring(L"chrome");
    browser->exeName = std::wstring(L"chrome.exe");
    browser->displayName = std::string("Google Chrome");

    // Get file version info
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(execFilePath.c_str(), &dummy);

    if (size == 0) {
        throw std::runtime_error("Failed to get file version info size for " + Core::ToUtf8(execFilePath));
    }

    char* buffer = new char[size];
    if (GetFileVersionInfoW(execFilePath.c_str(), 0, size, buffer)) {
        // browser->version = Core::ToUtf8(std::wstring_view(reinterpret_cast<const wchar_t*>(buffer), size / sizeof(wchar_t)));
        browser->version = std::string("151.0.7922.174");
    }

    VS_FIXEDFILEINFO* fixedInfo = nullptr;
    UINT fixedLen = 0;
    if (VerQueryValueW(buffer, L"\\", (LPVOID*)&fixedInfo, &fixedLen) && fixedLen > 0) {
        std::wcout << L"\n[Binary Fixed Product Versions]" << std::endl;
        std::wcout << L"  Binary File Version: "
                   << HIWORD(fixedInfo->dwFileVersionMS) << L"."
                   << LOWORD(fixedInfo->dwFileVersionMS) << L"."
                   << HIWORD(fixedInfo->dwFileVersionLS) << L"."
                   << LOWORD(fixedInfo->dwFileVersionLS) << std::endl;
        std::wcout << L"  Binary Prod Version: "
                   << HIWORD(fixedInfo->dwProductVersionMS) << L"."
                   << LOWORD(fixedInfo->dwProductVersionMS) << L"."
                   << HIWORD(fixedInfo->dwProductVersionLS) << L"."
                   << LOWORD(fixedInfo->dwProductVersionLS) << std::endl;
    }

    // 4. Query the translation table for Lang & Code Page strings
    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;

    UINT cbTranslate = 0;
    if (VerQueryValueW(buffer, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
        // Loop through all languages compiled inside the executable
        for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
            std::wcout << L"\n[String Properties - Lang ID: " << lpTranslate[i].wLanguage 
                       << L", CodePage: " << lpTranslate[i].wCodePage << L"]" << std::endl;

            // Loop through our standard metadata matrix
            for (const auto& propertyKey : StandardProperties) {
                wchar_t subBlockPath[256];
                swprintf_s(subBlockPath, L"\\StringFileInfo\\%04x%04x\\%s",
                           lpTranslate[i].wLanguage, lpTranslate[i].wCodePage, propertyKey.c_str());
                wchar_t* propertyValue = nullptr;
                UINT valLen = 0;
                // Print the property if it exists and contains data
                if (VerQueryValueW(buffer, subBlockPath, (LPVOID*)&propertyValue, &valLen) && valLen > 0) {
                    std::wcout << L"  " << propertyKey << L": " << propertyValue << std::endl;
                }
            }
        }
    }

    delete[] buffer;
    return browser;
}


bool SerializeBrowserInfo(const BrowserInfo& browser, char* dst, size_t& written) {
    // Serialize the BrowserInfo structure into the provided buffer
    // Calculate the total size needed for serialization
    size_t size = sizeof(uint32_t) + browser.type.length() + sizeof(uint32_t) + browser.exeName.length() + sizeof(uint32_t) + browser.fullPath.length();

    if (dst == nullptr) {
        written = size;
        return true;
    }

    if (written < size) {
        return false;
    }

    char* ptr = dst;
    uint32_t typeLength = static_cast<uint32_t>(browser.type.length() * sizeof(wchar_t));
    std::memcpy(ptr, &typeLength, sizeof(typeLength));
    ptr += sizeof(typeLength);
    std::memcpy(ptr, browser.type.data(), browser.type.length());
    ptr += browser.type.length();

    uint32_t exeNameLength = static_cast<uint32_t>(browser.exeName.length() * sizeof(wchar_t));
    std::memcpy(ptr, &exeNameLength, sizeof(exeNameLength));
    ptr += sizeof(exeNameLength);
    std::memcpy(ptr, browser.exeName.data(), browser.exeName.length());
    ptr += browser.exeName.length();

    uint32_t fullPathLength = static_cast<uint32_t>(browser.fullPath.length() * sizeof(wchar_t));
    std::memcpy(ptr, &fullPathLength, sizeof(fullPathLength));
    ptr += sizeof(fullPathLength);
    std::memcpy(ptr, browser.fullPath.data(), browser.fullPath.length());
    ptr += browser.fullPath.length();

    written = size;
    return true;
}


BrowserInfo DeserializeBrowserInfo(const char* data, size_t& read) {
    BrowserInfo browser;
    const char* ptr = data;

    // casting first 4 bytes to get the length of the type string
    uint32_t typeLength = (uint32_t)(*(reinterpret_cast<const uint32_t*>(ptr)));
    ptr += sizeof(typeLength);
    browser.type = std::wstring(reinterpret_cast<const wchar_t*>(ptr), typeLength / sizeof(wchar_t));
    ptr += typeLength;

    // casting next 4 bytes to get the length of the exeName string
    uint32_t exeNameLength = (uint32_t)(*(reinterpret_cast<const uint32_t*>(ptr)));
    ptr += sizeof(exeNameLength);
    browser.exeName = std::wstring(reinterpret_cast<const wchar_t*>(ptr), exeNameLength / sizeof(wchar_t));
    ptr += exeNameLength;

    // casting next 4 bytes to get the length of the fullPath string
    uint32_t fullPathLength = (uint32_t)(*(reinterpret_cast<const uint32_t*>(ptr)));
    ptr += sizeof(fullPathLength);
    browser.fullPath = std::wstring(reinterpret_cast<const wchar_t*>(ptr), fullPathLength / sizeof(wchar_t));
    ptr += fullPathLength;

    read = ptr - data;

    return browser;
}


} // namespace Injector