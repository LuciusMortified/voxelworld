module;

#include <string_view>

export module vw.core:log;

import :types;

export namespace vw::log {

enum class level : uint8 {
    trace,
    debug,
    info,
    warn,
    error,
    critical,
    off,
};

// Calls below this are discarded at compile time, argument formatting included.
// Set through the VW_LOG_MIN_LEVEL cache variable; the macro is private to this
// translation unit, so consumers see a plain constant and cannot disagree on it.
inline constexpr level min_level = level::VW_LOG_MIN_LEVEL;

void set_level(level lvl);
[[nodiscard]] auto get_level() -> level;
void add_file_sink(std::string_view path);
void write(level lvl, std::string_view category, std::string_view message);

}  // namespace vw::log
