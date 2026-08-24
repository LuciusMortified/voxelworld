export module vw.sculptor:tools;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;
import :state;
import :operations;

// ---- from src/tools/base_tool.h
export namespace vw::sculptor {

class base_tool {
public:
    virtual ~base_tool() = default;

    virtual auto render(float delta_time) -> void = 0;

    virtual auto on_key_press(const plat::key_press_event& ev) -> void         = 0;
    virtual auto on_mouse_move(const plat::mouse_move_event& ev) -> void       = 0;
    virtual auto on_mouse_press(const plat::mouse_press_event& ev) -> void     = 0;
    virtual auto on_mouse_release(const plat::mouse_release_event& ev) -> void = 0;

    virtual auto on_activate() -> void = 0;
};

}  // namespace vw::sculptor

// ---- from src/tools/add_voxel_tool.h
export namespace vw::sculptor {

class add_voxel_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    add_voxel_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    auto render(float delta_time) -> void override;
    auto on_key_press(const plat::key_press_event& ev) -> void override;
    auto on_mouse_move(const plat::mouse_move_event& ev) -> void override;
    auto on_mouse_press(const plat::mouse_press_event& ev) -> void override;
    auto on_mouse_release(const plat::mouse_release_event& ev) -> void override;
    auto on_activate() -> void override;

private:
    auto update_hovered_voxel_() -> void;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::vector<ecs::entity> ray_cast_entities_;
    vec3i hovered_voxel_ = vec3i{-1, -1, -1};
};

}  // namespace vw::sculptor

// ---- from src/tools/color_picker_tool.h
export namespace vw::sculptor {

class color_picker_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    color_picker_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    auto render(float delta_time) -> void override;
    auto on_key_press(const plat::key_press_event& ev) -> void override;
    auto on_mouse_move(const plat::mouse_move_event& ev) -> void override;
    auto on_mouse_press(const plat::mouse_press_event& ev) -> void override;
    auto on_mouse_release(const plat::mouse_release_event& ev) -> void override;
    auto on_activate() -> void override;

private:
    auto update_hovered_voxel_() -> void;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::vector<ecs::entity> ray_cast_entities_;
    vec3i hovered_voxel_ = vec3i{-1, -1, -1};
};

}  // namespace vw::sculptor

// ---- from src/tools/dummy_tool.h
export namespace vw::sculptor {

class dummy_tool final : public base_tool {
public:
    auto render(
        [[maybe_unused]] float delta_time
    ) -> void override{}

    auto on_key_press(
        [[maybe_unused]] const plat::key_press_event& ev
    ) -> void override{}

    auto on_mouse_move(
        [[maybe_unused]] const plat::mouse_move_event& ev
    ) -> void override{}

    auto on_mouse_press(
        [[maybe_unused]] const plat::mouse_press_event& ev
    ) -> void override{}

    auto on_mouse_release(
        [[maybe_unused]] const plat::mouse_release_event& ev
    ) -> void override{}

    auto on_activate() -> void override{}
};

}  // namespace vw::sculptor

// ---- from src/tools/paint_tool.h
export namespace vw::sculptor {

class paint_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    paint_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    auto render(float delta_time) -> void override;
    auto on_key_press(const plat::key_press_event& ev) -> void override;
    auto on_mouse_move(const plat::mouse_move_event& ev) -> void override;
    auto on_mouse_press(const plat::mouse_press_event& ev) -> void override;
    auto on_mouse_release(const plat::mouse_release_event& ev) -> void override;
    auto on_activate() -> void override;

private:
    auto update_hovered_voxel_() -> void;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::vector<ecs::entity> ray_cast_entities_;
    vec3i hovered_voxel_ = vec3i{-1, -1, -1};
};

}  // namespace vw::sculptor

// ---- from src/tools/remove_voxel_tool.h
export namespace vw::sculptor {

class remove_voxel_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    remove_voxel_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    auto render(float delta_time) -> void override;
    auto on_key_press(const plat::key_press_event& ev) -> void override;
    auto on_mouse_move(const plat::mouse_move_event& ev) -> void override;
    auto on_mouse_press(const plat::mouse_press_event& ev) -> void override;
    auto on_mouse_release(const plat::mouse_release_event& ev) -> void override;
    auto on_activate() -> void override;

private:
    auto update_hovered_voxel_() -> void;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::vector<ecs::entity> ray_cast_entities_;
    vec3i hovered_voxel_ = vec3i{-1, -1, -1};
};

}  // namespace vw::sculptor

// ---- from src/tools/select_entity_tool.h
export namespace vw::sculptor {

class select_entity_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    select_entity_tool(engine_type& eng, app_state& st);

    auto render(float delta_time) -> void override;
    auto on_key_press(const plat::key_press_event& ev) -> void override;
    auto on_mouse_move(const plat::mouse_move_event& ev) -> void override;
    auto on_mouse_press(const plat::mouse_press_event& ev) -> void override;
    auto on_mouse_release(const plat::mouse_release_event& ev) -> void override;
    auto on_activate() -> void override;

private:
    auto update_hovered_entity_() -> void;
    auto draw_entity_box_(ecs::entity ent, color col) -> void;

    engine_type* engine_;
    app_state* state_;

    std::vector<ecs::entity> ray_cast_entities_;
    ecs::entity hovered_entity_;
};

}  // namespace vw::sculptor
