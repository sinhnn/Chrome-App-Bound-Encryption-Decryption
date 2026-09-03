#pragma once

#include <vector>
#include <string>
#include "browser_discovery.hpp"

namespace Injector {
    BrowserInfo* CreateBrowserInfoFromExecFilePath(const std::wstring& execPath);
    bool SerializeBrowserInfo(const BrowserInfo& browser, char* dst, size_t& written);
    BrowserInfo DeserializeBrowserInfo(const char* data, size_t& read);
} // namespace Injector