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

void set_level(level lvl);
[[nodiscard]] auto get_level() -> level;
void add_file_sink(std::string_view path);
void write(level lvl, std::string_view category, std::string_view message);

}  // namespace vw::log
