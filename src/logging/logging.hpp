#include <chrono>
#include <iomanip>
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
        std::time_t result = std::time(nullptr);
        std::cout << result << " [" << std::setw(7) << ToString(type) << "] ";
    }
    template <typename Type>
    Logger& operator<<(Type&& value)
    {
        std::cout << value;
        return *this;
    }
    ~Logger()
    {
        std::cout << "\n";
    }
};

} // namespace echo_reverse_server::logging

#define INTERNAL_LOG(level) echo_reverse_server::logging::Logger(level)

#define LOG_INFO() INTERNAL_LOG(echo_reverse_server::logging::LoggerType::Info)
#define LOG_WARNING() INTERNAL_LOG(echo_reverse_server::logging::LoggerType::Warning)
#define LOG_ERROR() INTERNAL_LOG(echo_reverse_server::logging::LoggerType::Error)
#define LOG_CRITICAL() INTERNAL_LOG(echo_reverse_server::logging::LoggerType::Critical)
