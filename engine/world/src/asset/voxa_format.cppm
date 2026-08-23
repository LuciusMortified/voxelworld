export module vw.world:serial.voxa;

import std;

import vw.core;
import :anim;
import :model;
import :serial.vox;

export namespace vw::asset {

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

}  // namespace vw::asset
