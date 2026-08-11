module;

#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

export module vw.world:serial;

import vw.core;
import :anim;
import :model;

export namespace vw::asset {

struct vox_socket_data {
    std::string name;
    vec3f position;
    vec3f rotation;
    vec3f scale;
};

struct vox_model_data {
    vec3i size;
    std::vector<std::pair<vec3i, voxel>> voxels;
};

struct vox_entity_data {
    std::string name;
    std::string parent_name;

    vec3f position;
    vec3f rotation;
    vec3f scale{1.0F, 1.0F, 1.0F};
    vec3f origin;
    bool has_transform = false;

    std::optional<vox_model_data> model;
    std::optional<std::string> animation_target_name;
    std::vector<vox_socket_data> sockets;
    bool has_sockets = false;
};

struct vox_prefab_data {
    std::string root_name;
    std::vector<vox_entity_data> entities;
};

// Base class for .vox format parsers.
class vox_parser {
public:
    enum class error_type : uint8 { file_open_failed, parse_error };

    virtual ~vox_parser() = default;

    virtual auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> = 0;
};

// Plain text format .vox parser.
class vox_parser_plain final : public vox_parser {
public:
    explicit vox_parser_plain(const block_registry& block_registry);

    auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> override;

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

    const block_registry* block_registry_;
    vox_prefab_data prefab_;
    vox_entity_data* current_entity_ = nullptr;
    std::optional<error_type> error_;
};

inline constexpr std::string_view voxa_file_version = "1.0";

class voxa_serializer final {
public:
    enum class error_type : uint8 { file_open_failed, write_failed };

    explicit voxa_serializer(const animation_clip& clip);

    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

private:
    void write_header_(std::ofstream& file);
    void write_track_(std::ofstream& file, const animation_track& track);
    void write_channel_(std::ofstream& file, const animation_channel_variant& channel);
    void write_keyframes_vec3f_(std::ofstream& file, const animation_channel<vec3f>& ch);
    void write_keyframes_quat_(std::ofstream& file, const animation_channel<quat>& ch);

    static auto interp_to_string_(math::interpolation_type interp) -> std::string_view;

    const animation_clip* clip_;
};

class voxa_deserializer final {
public:
    enum class error_type : uint8 { file_open_failed, parse_error };

    auto deserialize(const std::filesystem::path& filepath)
        -> std::expected<std::shared_ptr<animation_clip>, error_type>;

private:
    void process_clip_(std::istringstream& iss);
    void process_track_(std::istringstream& iss);
    void process_channel_(std::istringstream& iss);
    void process_keyframe_(std::istringstream& iss);
    void finalize_channel_();
    void finalize_track_();

    static auto string_to_interp_(const std::string& s) -> math::interpolation_type;

    std::shared_ptr<animation_clip> clip_;
    std::optional<error_type> error_;

    std::unique_ptr<animation_track> current_track_;
    animation_property current_property_ = animation_property::position;
    bool has_current_channel_            = false;
    bool current_channel_is_quat_        = false;

    std::vector<keyframe<vec3f>> vec3f_keyframes_;
    std::vector<keyframe<quat>> quat_keyframes_;
};

// Stores loaded prefabs (models + metadata) and animation clips.
class asset_storage final {
public:
    asset_storage(vox_parser& parser, model_registry& registry);

    void load_prefab(std::string_view name, const std::filesystem::path& filepath);
    void load_clip(std::string_view name, const std::filesystem::path& filepath);

    [[nodiscard]] auto get_entity(std::string_view prefab, std::string_view entity_name) const
        -> const vox_entity_data&;

    [[nodiscard]] auto get_model(std::string_view prefab, std::string_view entity_name) const
        -> std::shared_ptr<model>;

    [[nodiscard]] auto get_clip(std::string_view name) const -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto has_clip(std::string_view name) const -> bool;

private:
    vox_parser* parser_;
    model_registry* model_registry_;
    std::unordered_map<std::string, vox_prefab_data> prefabs_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
    std::unordered_map<std::string, std::shared_ptr<animation_clip>> clips_;
};

}  // namespace vw::asset

export namespace vw::ecs {

inline constexpr std::string_view vox_file_version = "1.0";

// Base class for .vox format writers.
class vox_writer {
public:
    enum class error_type : uint8 { file_open_failed, write_failed };

    virtual ~vox_writer() = default;

    virtual auto write(const std::filesystem::path& filepath,
                       const asset::vox_prefab_data& prefab) -> std::expected<void, error_type> = 0;
};

// Plain text format .vox writer.
class vox_writer_plain final : public vox_writer {
public:
    auto write(const std::filesystem::path& filepath, const asset::vox_prefab_data& prefab)
        -> std::expected<void, error_type> override;

private:
    void write_header_(std::ofstream& file, const asset::vox_prefab_data& prefab);
    void write_entity_(std::ofstream& file, const asset::vox_entity_data& ent);
    void write_model_(std::ofstream& file, const asset::vox_model_data& mdl);
};

}  // namespace vw::ecs
