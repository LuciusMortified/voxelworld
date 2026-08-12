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

    virtual void render(float delta_time) = 0;

    virtual void on_key_press(const plat::key_press_event& ev)         = 0;
    virtual void on_mouse_move(const plat::mouse_move_event& ev)       = 0;
    virtual void on_mouse_press(const plat::mouse_press_event& ev)     = 0;
    virtual void on_mouse_release(const plat::mouse_release_event& ev) = 0;

    virtual void on_activate() = 0;
};

}  // namespace vw::sculptor

// ---- from src/tools/add_voxel_tool.h
export namespace vw::sculptor {

class add_voxel_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    add_voxel_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time) override;
    void on_key_press(const plat::key_press_event& ev) override;
    void on_mouse_move(const plat::mouse_move_event& ev) override;
    void on_mouse_press(const plat::mouse_press_event& ev) override;
    void on_mouse_release(const plat::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_voxel_();

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

    void render(float delta_time) override;
    void on_key_press(const plat::key_press_event& ev) override;
    void on_mouse_move(const plat::mouse_move_event& ev) override;
    void on_mouse_press(const plat::mouse_press_event& ev) override;
    void on_mouse_release(const plat::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_voxel_();

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
    void render(
        float delta_time
    ) override {}

    void on_key_press(
        const plat::key_press_event& ev
    ) override {}

    void on_mouse_move(
        const plat::mouse_move_event& ev
    ) override {}

    void on_mouse_press(
        const plat::mouse_press_event& ev
    ) override {}

    void on_mouse_release(
        const plat::mouse_release_event& ev
    ) override {}

    void on_activate() override {}
};

}  // namespace vw::sculptor

// ---- from src/tools/paint_tool.h
export namespace vw::sculptor {

class paint_tool final : public base_tool {
public:
    using engine_type = gfx::engine;

    paint_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time) override;
    void on_key_press(const plat::key_press_event& ev) override;
    void on_mouse_move(const plat::mouse_move_event& ev) override;
    void on_mouse_press(const plat::mouse_press_event& ev) override;
    void on_mouse_release(const plat::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_voxel_();

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

    void render(float delta_time) override;
    void on_key_press(const plat::key_press_event& ev) override;
    void on_mouse_move(const plat::mouse_move_event& ev) override;
    void on_mouse_press(const plat::mouse_press_event& ev) override;
    void on_mouse_release(const plat::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_voxel_();

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

    void render(float delta_time) override;
    void on_key_press(const plat::key_press_event& ev) override;
    void on_mouse_move(const plat::mouse_move_event& ev) override;
    void on_mouse_press(const plat::mouse_press_event& ev) override;
    void on_mouse_release(const plat::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_entity_();
    void draw_entity_box_(ecs::entity ent, color col);

    engine_type* engine_;
    app_state* state_;

    std::vector<ecs::entity> ray_cast_entities_;
    ecs::entity hovered_entity_;
};

}  // namespace vw::sculptor
