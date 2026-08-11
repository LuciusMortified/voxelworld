module vw.platform:logging;

import std;
import vw.core;

// Same implementation partition as vw.world:logging, for the same reason: a
// module unit cannot include vw/log/logger.h, and `import std` is only safe
// where no consumer can see it. Both copies collapse into vw.core in M6.
namespace vw::log {

struct log_category {
    std::string_view value;

    constexpr explicit log_category(std::string_view cat) : value(cat) {}
    constexpr explicit log_category(const char* cat) : value(cat) {}
};

namespace detail {

template <level Lvl, typename... Args>
void emit(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if constexpr (Lvl >= min_level) {
        if (Lvl >= get_level()) {
            write(Lvl, category, std::format(fmt, std::forward<Args>(args)...));
        }
    }
}

}  // namespace detail

template <typename... Args>
void trace(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::trace>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::debug>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::info>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::warn>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::error>(cat.value, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(log_category cat, std::format_string<Args...> fmt, Args&&... args) {
    detail::emit<level::critical>(cat.value, fmt, std::forward<Args>(args)...);
}

}  // namespace vw::log
