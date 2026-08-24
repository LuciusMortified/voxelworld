#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.world;

using namespace vw;

namespace {

auto parse_vox(std::string_view text) {
    static const block_registry registry;

    std::istringstream input{std::string{text}};
    asset::vox_parser_plain parser{registry};
    return parser.parse(input);
}

auto parse_voxa(std::string_view text) {
    std::istringstream input{std::string{text}};
    asset::voxa_deserializer deserializer;
    return deserializer.deserialize(input);
}

}  // namespace

TEST_CASE("the vox parser reads a prefab out of a stream", "[serial]") {
    const auto prefab = parse_vox(
        "# comment\n"
        "root body\n"
        "entity body\n"
        "\tt 1 2 3\t0 0 0\t1 1 1\t0 0 0\n"
        "\tm 2 2 2\n"
        "\t\tv 0 0 0 0x13\n"
        "\t\tv 1 1 1 0x2E\n"
    );

    REQUIRE(prefab.has_value());
    REQUIRE(prefab->root_name == "body");
    REQUIRE(prefab->entities.size() == 1);

    const auto& entity = prefab->entities.front();
    REQUIRE(entity.name == "body");
    REQUIRE(entity.has_transform);
    REQUIRE(entity.position == vec3f{1.0F, 2.0F, 3.0F});

    REQUIRE(entity.model.has_value());
    REQUIRE(entity.model->size == vec3i{2, 2, 2});
    REQUIRE(entity.model->voxels.size() == 2);
}

// Разбор идёт построчно, и незнакомая команда — не повод бросать файл: так
// формат остаётся расширяемым, а старый разборщик читает новый файл.
TEST_CASE("an unknown vox command is skipped, not fatal", "[serial]") {
    const auto prefab = parse_vox(
        "root body\n"
        "entity body\n"
        "\tqqq 1 2 3\n"
    );

    REQUIRE(prefab.has_value());
    REQUIRE(prefab->entities.size() == 1);
}

// Обрезанная строка — то, что приносит и оборванная запись, и правка руками.
// Ответ обязан быть ошибкой разбора, а не догадкой о недостающих числах.
TEST_CASE("a truncated vox transform is a parse error", "[serial]") {
    const auto prefab = parse_vox(
        "root body\n"
        "entity body\n"
        "\tt 1 2\n"
    );

    REQUIRE_FALSE(prefab.has_value());
    REQUIRE(prefab.error() == asset::vox_parser::error_type::parse_error);
}

TEST_CASE("an empty vox stream yields an empty prefab", "[serial]") {
    const auto prefab = parse_vox("");

    REQUIRE(prefab.has_value());
    REQUIRE(prefab->entities.empty());
}

TEST_CASE("the voxa parser reads a clip out of a stream", "[serial]") {
    const auto clip = parse_voxa(
        "# comment\n"
        "clip walk\n"
        "track body 60\n"
        "  channel position\n"
        "    k 0 0 0 0 linear 0 1\n"
        "    k 1.5 0 1 0 linear 0 1\n"
    );

    REQUIRE(clip.has_value());
    REQUIRE(*clip != nullptr);
    REQUIRE((*clip)->get_name() == "walk");
    REQUIRE((*clip)->has_track("body"));
    REQUIRE((*clip)->get_tracks().size() == 1);
}

TEST_CASE("a truncated voxa keyframe is a parse error", "[serial]") {
    const auto clip = parse_voxa(
        "clip walk\n"
        "track body 60\n"
        "  channel position\n"
        "    k 0 0\n"
    );

    REQUIRE_FALSE(clip.has_value());
    REQUIRE(clip.error() == asset::voxa_deserializer::error_type::parse_error);
}

// То, что перебирает фаззер, но в виде, который читается глазами: разборщик
// обязан дойти до конца любого ввода и вернуть либо клип, либо ошибку.
TEST_CASE("the parsers survive rubbish", "[serial]") {
    const std::array<std::string_view, 6> rubbish{
        "\0\0\0", "clip", "track", "k k k", "v v v", "root\n\n\nentity\n",
    };

    for (const auto& text : rubbish) {
        static_cast<void>(parse_vox(text));
        static_cast<void>(parse_voxa(text));
    }

    SUCCEED("neither parser crashed");
}
