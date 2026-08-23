export module vw.world:serial.vox;

import std;

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

// База для разборщиков формата .vox.
class vox_parser {
public:
    enum class error_type : uint8 { file_open_failed, parse_error };

    virtual ~vox_parser() = default;

    virtual auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> = 0;
};

// Разборщик текстового варианта .vox.
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

}  // namespace vw::asset
