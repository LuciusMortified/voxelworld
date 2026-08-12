export module vw.sculptor:services;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;
import :state;
import :operations;

// ---- from src/services/clip_service.h
export namespace vw::sculptor {

class clip_service final {
public:
    using engine_type = gfx::engine;

    clip_service(engine_type& eng, app_state& state, operation_manager& op_manager);

    auto save_clip(const std::string& clip_name) const -> bool;
    auto save_clip_as(const std::string& clip_name, const std::string& new_name) const -> bool;
    void save_all_clips() const;
    auto load_clip(const std::string& filename) const -> bool;
    void close_clip(const std::string& clip_name) const;

    void enter_animation_mode();
    void exit_animation_mode();
    void force_exit_animation_mode();

    void save_transforms();
    void restore_transforms();
    void reset_all();

    void stop_layer_for_clip(const std::string& clip_name);
    void stop_all_layers();

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

// ---- from src/services/file_service.h
export namespace vw::sculptor {

class file_service final {
public:
    using engine_type = gfx::engine;

    file_service(engine_type& eng, app_state& state);

    auto save() -> bool;
    auto save_as(const std::filesystem::path& filepath) -> bool;

private:
    engine_type* engine_;
    app_state* state_;
};

}  // namespace vw::sculptor

// ---- from src/services/keyframe_service.h
export namespace vw::sculptor {

class keyframe_service final {
public:
    using engine_type = gfx::engine;

    keyframe_service(engine_type& eng, app_state& state, operation_manager& op_manager);

    void add_keyframe();
    void delete_keyframe();

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

// ---- from src/services/playback_service.h
export namespace vw::sculptor {

class playback_service final {
public:
    using engine_type = gfx::engine;

    playback_service(engine_type& eng, app_state& state);

    void toggle_playback() const;
    void stop_playback() const;

private:
    engine_type* engine_;
    app_state* state_;
};

}  // namespace vw::sculptor
