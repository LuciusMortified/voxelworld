#pragma once

#ifndef VW_GFX_VOX_DESERIALIZER_H
#define VW_GFX_VOX_DESERIALIZER_H
#include <expected>
#include <filesystem>
#include <optional>

#include "vw/gfx/world/world.h"

namespace vw::gfx {
template <typename WC = base_world_components>
class vox_deserializer final {
public:
    using world_type        = world<WC>;
    using entity_guard_type = entity_guard<WC>;

    enum class error_type : uint8 { file_open_failed, parse_error };

    struct options {
        bool skip_sockets = false;
        bool skip_targets = false;
    };

    struct result {
        std::string root_name;
        std::unordered_map<std::string, entity> name_to_entity;
        std::unordered_map<entity, std::string> entity_to_name;
        std::vector<std::unique_ptr<entity_guard_type>> entities;
    };

    vox_deserializer(world_type& world);

    auto deserialize(const std::filesystem::path& filepath, const options& opts = {})
        -> std::expected<result, error_type>;

private:
    void process_root_(std::istringstream& iss);
    void process_entity_(std::istringstream& iss);
    void process_parent_(std::istringstream& iss);
    void process_transform_(std::istringstream& iss);
    void process_target_(std::istringstream& iss);
    void process_sockets_();
    void process_socket_(std::istringstream& iss);
    void process_model_(std::istringstream& iss);
    void process_voxel_(std::istringstream& iss);

    world_type* world_;
    options options_;

    result result_;
    std::optional<error_type> error_;

    entity current_entity_ = invalid_entity;
    std::unique_ptr<entity_guard_type> current_entity_guard_;
    std::shared_ptr<model> current_model_;
};

}  // namespace vw::gfx

#include "vox_deserializer.inl.h"

#endif  // VW_GFX_VOX_DESERIALIZER_H
