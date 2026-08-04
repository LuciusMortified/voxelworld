#pragma once

#ifndef VW_LOG_LOGGER_H
#define VW_LOG_LOGGER_H

#include <format>
#include <string_view>
#include <utility>

#include "vw/config.h"
#include "vw/core/detail/module_prelude.h"

import vw.core;

// The formatting layer stays a header until M6: a module interface that pulls
// <format> through its global module fragment collides with consumers that
// still include the same headers textually. Only the sink lives in vw.core.
namespace vw::log {

struct log_category {
    std::string_view value;

    constexpr explicit log_category(std::string_view cat) : value(cat) {}
    constexpr explicit log_category(const char* cat) : value(cat) {}
};

namespace detail {

template <level Lvl, typename... Args>
void emit(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if constexpr (static_cast<int>(Lvl) >= VW_LOG_MIN_LEVEL) {
        if (Lvl >= get_level()) {
            write(Lvl, category, std::format(fmt, std::forward<Args>(args)...));
        }
    }
}

}  // namespace detail

template <typename... Args>
void trace(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::trace>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void trace(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::trace>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::debug>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::debug>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::info>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::info>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::warn>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::warn>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::error>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::error>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::critical>({}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::critical>(cat.value, fmt, std::forward<Args>(args)...);
}

}  // namespace vw::log

#endif  // VW_LOG_LOGGER_H
