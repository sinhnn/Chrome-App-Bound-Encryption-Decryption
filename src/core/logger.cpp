#include "logger.hpp"

namespace Core {

bool ILogger::is_enabled_for(int level) {
    return level >= this->level;
}

wchar_t* ILogger::get_timestamp() {
    static wchar_t buffer[64];
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    std::wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
    return buffer;
}

wchar_t* ILogger::get_prefix(int level) {
    // get prefix with timestamp and log level
    static wchar_t buffer[128];
    std::swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%ls [%ls] ", get_timestamp(),
                  level == ILogger::LOGLEVEL_DEBUG ? L"DEBUG" :
                  level == ILogger::LOGLEVEL_INFO ? L"INFO" :
                  level == ILogger::LOGLEVEL_WARNING ? L"WARNING" :
                  level == ILogger::LOGLEVEL_ERROR ? L"ERROR" : L"UNKNOWN");
    return buffer;

}

void ILogger::debug(const std::wstring &message) {
    if (!is_enabled_for(ILogger::LOGLEVEL_DEBUG)) return;
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_DEBUG)) + message);
}

void ILogger::info(const std::wstring &message) {
    if (!is_enabled_for(ILogger::LOGLEVEL_INFO)) return;
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_INFO)) + message);
}

void ILogger::warning(const std::wstring &message) {
    if (!is_enabled_for(ILogger::LOGLEVEL_WARNING)) return;
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_WARNING)) + message);
}

void ILogger::error(const std::wstring &message) {
    if (!is_enabled_for(ILogger::LOGLEVEL_ERROR)) return;
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_ERROR)) + message);
}

void ILogger::log(const std::wstring &prefix, const wchar_t *format, ...) {
    va_list args;
    va_start(args, format);
    log(prefix.c_str(), format, args);
    va_end(args);
}

void ILogger::debug(const wchar_t *format, ...) {
    if (!is_enabled_for(ILogger::LOGLEVEL_DEBUG)) return;
    va_list args;
    va_start(args, format);
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_DEBUG)), format, args);
    va_end(args);
}

void ILogger::info(const wchar_t *format, ...) {
    if (!is_enabled_for(ILogger::LOGLEVEL_INFO)) return;
    va_list args;
    va_start(args, format);
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_INFO)), format, args);
    va_end(args);
}

void ILogger::warning(const wchar_t *format, ...) {
    if (!is_enabled_for(ILogger::LOGLEVEL_WARNING)) return;
    va_list args;
    va_start(args, format);
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_WARNING)), format, args);
    va_end(args);
}

void ILogger::error(const wchar_t *format, ...) {
    if (!is_enabled_for(ILogger::LOGLEVEL_ERROR)) return;
    va_list args;
    va_start(args, format);
    log(std::wstring(get_prefix(ILogger::LOGLEVEL_ERROR)), format, args);
    va_end(args);
}

Logger& Logger::instance() {
    static Logger instance(stdout);
    return instance;
}

Logger::Logger(FILE *writer) : writer_(writer), owns_writer_(false) {}
Logger::Logger() : writer_(stdout), owns_writer_(false) {}
Logger::Logger(const std::wstring &path, const wchar_t *mode) : writer_(nullptr) {
    writer_ = _wfopen(path.c_str(), mode);
    if (!writer_) {
        writer_ = stdout;
        owns_writer_ = false;
    } else {
        owns_writer_ = true;
    }
}

void Logger::log(const std::wstring &message)
{
    fwprintf(this->writer_, L"%ls\n", message.c_str());
    // DisableThreadLibraryCalls(hModule) in injection code requires to flush the output manually.
    fflush(this->writer_);
}

void Logger::log(const wchar_t *prefix, const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    fwprintf(this->writer_, L"%ls", prefix);
    vfwprintf(this->writer_, format, args);
    fwprintf(this->writer_, L"\n");
    // DisableThreadLibraryCalls(hModule) in injection code requires to flush the output manually.
    fflush(this->writer_);
    va_end(args);
}

Logger::~Logger() {
    if (owns_writer_ && writer_) {
        fclose(writer_);
    }
}
} // namespace Core