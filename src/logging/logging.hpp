#include <iostream>

namespace echo_reverse_server::logging {

enum class LoggerType {
    Info,
    Warning,
    Error,
    Critical,
};

constexpr auto ToString(LoggerType type)
{
    switch (type) {
    case LoggerType::Info:
        return "INFO";
    case LoggerType::Warning:
        return "WARN";
    case LoggerType::Error:
        return "ERROR";
    case LoggerType::Critical:
        return "CRIT";
    default:
        return "UNEXPECTED";
    }
}

struct Logger {
    Logger(LoggerType type)
    {
        std::cout << "[" << ToString(type) << "]\t";
    }
    template <typename Type>
    Logger& operator<<(Type&& value)
    {
        std::cout << value;
        return *this;
    }
    ~Logger() { std::cout << "\n"; }
};

} // namespace echo_reverse_server::logging

#define LOG_INFO() echo_reverse_server::logging::Logger(echo_reverse_server::logging::LoggerType::Info)
#define LOG_WARNING() echo_reverse_server::logging::Logger(echo_reverse_server::logging::LoggerType::Warning)
#define LOG_ERROR() echo_reverse_server::logging::Logger(echo_reverse_server::logging::LoggerType::Error)
#define LOG_CRITICAL() echo_reverse_server::logging::Logger(echo_reverse_server::logging::LoggerType::Critical)
