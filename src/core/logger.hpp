#pragma once
#include <cstdarg>
#include <string>
#include <sstream>

namespace Core
{
    class ILogger {
    public:
        static constexpr int LOGLEVEL_DEBUG = 0;
        static constexpr int LOGLEVEL_INFO = 10;
        static constexpr int LOGLEVEL_WARNING = 20;
        static constexpr int LOGLEVEL_ERROR = 30;
        static wchar_t* get_timestamp();

        virtual ~ILogger() = default;
        void debug(const std::wstring &message);
        void info(const std::wstring &message);
        void warning(const std::wstring &message);
        void error(const std::wstring &message);

        void debug(const wchar_t *format, ...);
        void info(const wchar_t *format, ...);
        void warning(const wchar_t *format, ...);
        void error(const wchar_t *format, ...);

        bool is_enabled_for(int level);
        int level = LOGLEVEL_DEBUG;
    protected:
        virtual void log(const std::wstring &message) = 0;
        virtual void log(const wchar_t *prefix, const wchar_t *format, ...) = 0;
        void log(const std::wstring &prefix, const wchar_t *format, ...);

        static wchar_t* get_prefix(int level);
    };

    class Logger : public ILogger
    {
    public:
        static Logger& instance();
        Logger();
        Logger(FILE *writer);
        Logger(const std::wstring &path, const wchar_t *mode = L"w") ;
        ~Logger();
    protected:
        void log(const std::wstring &message) override;
        void log(const wchar_t *prefix, const wchar_t *format, ...) override;

    private:
        FILE *writer_;
        bool owns_writer_;
    };
}