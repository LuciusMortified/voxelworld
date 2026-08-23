export module vw.world:serial.scene;

import std;

import vw.core;
import vw.ecs;
import :anim;
import :model;
import :components;
import :serial.vox;
import :serial.writer;

export namespace vw::ecs {

class world;

class vox_serializer final {
public:
    using entity_names_type = std::unordered_map<entity, std::string>;
    using error_type        = vox_writer::error_type;

    struct options {
        std::optional<entity_names_type> entity_names;
        std::unordered_set<entity> excluded;
    };

    vox_serializer(world& world, vox_writer& writer, entity root, options opts = {});

    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

    [[nodiscard]] auto extract() const -> asset::vox_prefab_data;

private:
    void generate_entity_names_();
    [[nodiscard]] auto extract_entity_(entity ent) const -> asset::vox_entity_data;

    world* world_;
    vox_writer* writer_;
    entity root_;
    entity_names_type entity_names_;
    std::unordered_set<entity> excluded_;
};

class vox_deserializer final {
public:
    using error_type = asset::vox_parser::error_type;

    struct options {
        bool skip_sockets = false;
        bool skip_targets = false;
    };

    struct result {
        std::string root_name;
        std::unordered_map<std::string, entity> name_to_entity;
        std::unordered_map<entity, std::string> entity_to_name;
        std::vector<entity> entities;
    };

    vox_deserializer(world& world, asset::vox_parser& parser);

    auto deserialize(const std::filesystem::path& filepath) -> std::expected<result, error_type>;
    auto deserialize(const std::filesystem::path& filepath, const options& opts)
        -> std::expected<result, error_type>;

private:
    void apply_entity_(const asset::vox_entity_data& data, result& res, const options& opts);

    world* world_;
    asset::vox_parser* parser_;
};

}  // namespace vw::ecs
