#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "vw/log/logger.h"

namespace {

auto sink_path(std::string_view name) -> std::filesystem::path {
    return std::filesystem::temp_directory_path() / name;
}

auto read_all(const std::filesystem::path& path) -> std::string {
    std::ifstream file{path};
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

}  // namespace

TEST_CASE("log: file sink records level, category and message", "[log]") {
    const auto path = sink_path("vw_log_sink.log");
    vw::log::set_level(vw::log::level::trace);
    vw::log::add_file_sink(path.string());

    vw::log::info("plain {}", 42);
    vw::log::warn(vw::log::log_category{"parser"}, "value={}", 7);

    const auto contents = read_all(path);

    REQUIRE(contents.find("[info] plain 42") != std::string::npos);
    REQUIRE(contents.find("[warning] [parser] value=7") != std::string::npos);
}

TEST_CASE("log: runtime level discards lower severities", "[log]") {
    const auto path = sink_path("vw_log_level.log");
    vw::log::add_file_sink(path.string());
    vw::log::set_level(vw::log::level::warn);

    vw::log::info("must not appear");
    vw::log::error("must appear");

    const auto contents = read_all(path);
    vw::log::set_level(vw::log::level::trace);

    REQUIRE(contents.find("must not appear") == std::string::npos);
    REQUIRE(contents.find("must appear") != std::string::npos);
}

TEST_CASE("log: timestamp prefix has millisecond precision", "[log]") {
    const auto path = sink_path("vw_log_stamp.log");
    vw::log::set_level(vw::log::level::trace);
    vw::log::add_file_sink(path.string());

    vw::log::info("stamped");

    const auto contents = read_all(path);

    REQUIRE(contents.size() > 25);
    REQUIRE(contents.front() == '[');
    REQUIRE(contents[5] == '-');
    REQUIRE(contents[8] == '-');
    REQUIRE(contents[14] == ':');
    REQUIRE(contents[17] == ':');
    REQUIRE(contents[20] == '.');
    REQUIRE(contents[24] == ']');
}
