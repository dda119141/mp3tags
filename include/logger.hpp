// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V2_LOGGER
#define ID3V2_LOGGER
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "spdlog/sinks/ostream_sink.h"

namespace id3
{
class Logger {
public:
    Logger(const std::string& logName) : max_size(1024 * 100) {
        logger = spdlog::rotating_logger_mt("logger", logName, max_size, 0);
        logger->set_pattern("[%H:%M:%S %z] [%^%L%$] %s:%!:%# %v");

         logger->set_level(spdlog::level::trace);
        // logger->set_level(spdlog::level::info);
        // logger->set_level(spdlog::level::debug);
        // logger->set_level(spdlog::level::warn);
        // logger->set_level(spdlog::level::err);
        // logger->set_level(spdlog::level::critical);
    }

    auto GetLog(void) const {
        logger->flush();
        return logger;
    }

private:
    std::shared_ptr<spdlog::logger> logger;
    uint32_t max_size;
};

template <typename... Args>
auto log(Args... args) {
    static id3::Logger tLogger;
    return tLogger.GetLog()->info(args...);
}

auto log() {
    static id3::Logger tLogger("id3.log");
    return tLogger.GetLog();
}

};  // end namespace id3

#define ID3_LOG_TRACE(...) SPDLOG_LOGGER_CALL(id3::log(), spdlog::level::trace, __VA_ARGS__)
#define ID3_LOG_INFO(...) SPDLOG_LOGGER_CALL(id3::log(), spdlog::level::info, __VA_ARGS__)
#define ID3_LOG_WARN(...) SPDLOG_LOGGER_CALL(id3::log(), spdlog::level::warn, __VA_ARGS__)
#define ID3_LOG_ERROR(...) SPDLOG_LOGGER_CALL(id3::log(), spdlog::level::err, __VA_ARGS__)

#endif //ID3V2_LOGGER
