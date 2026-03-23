#pragma once

#ifndef VW_LOG_LOGGER_H
#define VW_LOG_LOGGER_H

#include <format>
#include <string_view>

namespace vw::log {

struct log_category {
    std::string_view value;

    constexpr explicit log_category(
        std::string_view cat
    )
        : value(cat) {}
    constexpr explicit log_category(
        const char* cat
    )
        : value(cat) {}
};

class logger {
public:
    static logger& get();

    logger(const logger&)            = delete;
    logger& operator=(const logger&) = delete;

    template <typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void trace(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void debug(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void info(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void warn(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void error(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void critical(std::format_string<Args...> fmt, Args&&... args);

    template <typename... Args>
    void critical(log_category cat, std::format_string<Args...> fmt, Args&&... args);

    void add_file_sink(std::string_view path);

private:
    logger();
    ~logger();

    class impl;
    impl* pimpl_;
};

}  // namespace vw::log

#include "vw/log/logger.inl.h"

#endif  // VW_LOG_LOGGER_H
