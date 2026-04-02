#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/asset_storage.h"
#include "vw/gfx/camera/third_person_camera_controller.h"
#include "vw/gfx/player/player_input_controller.h"
#include "vw/gfx/player/third_person_player_controller.h"
#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world/serializers/vox_parser_plain.h"
#include "vw/gfx/world_grid/generators/perlin_terrain_generator.h"
#include "vw/gfx/world_grid/world_grid.h"

using namespace vw;

class player_controller_app final : public gfx::app<> {
public:
    explicit player_controller_app(
        gfx::engine<>& eng
    )
        : app{eng}
        , parser_(eng.get_block_registry())
        , assets_(parser_, eng.get_world().get_model_registry())
        , input_controller_(get_engine().get_window())
        , camera_controller_(
              get_engine().get_camera(),
              get_engine().get_world(),
              gfx::third_person_camera_params{
                  .arm_length     = 80.0f,
                  .arm_length_min = 10.0f,
                  .arm_length_max = 200.0f,
                  .target_offset  = {0.0f, 24.0f, 0.0f},
                  .pitch_min      = -30.0f,
                  .pitch_max      = 80.0f,
                  .zoom_speed     = 5.0f,
                  .collision_skin = 2.0f
              }
          )
        , player_controller_(get_engine().get_camera(), get_engine().get_world()) {
        auto& window = get_engine().get_window();
        window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) -> bool {
            handle_key_press(event.key);
            return true;
        });
        window.sub<gfx::mouse_press_event>([this](const gfx::mouse_press_event& event) -> bool {
            handle_mouse_press(event.button);
            return true;
        });
        window.sub<gfx::window_close_event>([this](gfx::window_close_event&) -> bool {
            get_engine().shutdown();
            return true;
        });

        get_engine().get_renderer().set_clear_color(0.4f, 0.6f, 0.9f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        input_controller_.set_mouse_captured(true);

        load_assets();
        setup_world_grid();
        setup_player();

        auto& fog         = get_engine().get_renderer().get_fog_settings();
        fog.color         = {0.4f, 0.6f, 0.9f};
        fog.near_distance = 6 * 64 * generator_params_.voxel_scale;
        fog.far_distance  = 9 * 64 * generator_params_.voxel_scale;
    }

    ~player_controller_app() override = default;

    void render(
        float delta_time
    ) override {
        try_place_player();

        const auto input = input_controller_.get_input_state();

        if (player_) {
            const auto player_ent = player_->get_entity();

            player_controller_.update(input, player_ent);
            camera_controller_.update(input, player_ent);

            auto& world    = get_engine().get_world();
            const auto& rb = world.get_component<gfx::rigid_body_component>(player_ent);

            if (input.jump_requested && rb.is_grounded()) {
                need_update_jump_ = true;
            }

            if (need_update_jump_ && !rb.is_grounded()) {
                jump_counter_     = (jump_counter_ + 1) % 2;
                need_update_jump_ = false;
            }
        }

        if (show_colliders_) {
            get_engine().get_renderer().draw_colliders(get_engine().get_world());
        }

        render_ui();
    }

private:
    void load_assets() {
        assets_.load_prefab("m_human", "assets/models/m_human.vox");
        assets_.load_prefab("m_sword", "assets/models/m_sword.vox");
        assets_.load_clip("a_idle", "assets/animations/a_idle_0.voxa");
        assets_.load_clip("a_walk", "assets/animations/a_walk_0.voxa");
        assets_.load_clip("a_jump_left", "assets/animations/a_jump_left_1.voxa");
        assets_.load_clip("a_jump_right", "assets/animations/a_jump_right_1.voxa");
        assets_.load_clip("a_sword_attack", "assets/animations/a_sword_attack_2.voxa");
    }

    auto create_body_part(
        const gfx::vox_entity_data& ent_data,
        const std::shared_ptr<gfx::model>& part_model,
        gfx::entity parent_ent
    ) -> std::unique_ptr<gfx::entity_guard<>> {
        auto& world            = get_engine().get_world();
        auto& hierarchy_system = world.get_hierarchy_system();
        auto& transform_system = world.get_transform_system();
        auto& model_system     = world.get_model_system();
        auto& animation_system = world.get_animation_system();

        auto guard = std::make_unique<gfx::entity_guard<>>(world);
        guard->with<gfx::hierarchy_component>();
        guard->with<gfx::transform_component>();
        guard->with<gfx::spatial_component>();
        guard->with<gfx::model_component>();
        guard->with<gfx::animation_target_component>();

        if (ent_data.has_sockets) {
            guard->with<gfx::socket_component>();
        }

        const auto ent = guard->get_entity();

        hierarchy_system.modify(ent).set_parent(parent_ent);

        transform_system.modify(ent)
            .set_position(ent_data.position)
            .set_rotation_euler(ent_data.rotation)
            .set_scale(ent_data.scale)
            .set_origin(ent_data.origin);

        model_system.modify(ent).set_model(part_model);

        animation_system.modify_target(ent).set_target_name(ent_data.name);

        if (ent_data.has_sockets) {
            auto& socket_system = world.get_socket_system();
            for (const auto& sp : ent_data.sockets) {
                socket_system.modify(ent).add_socket(
                    sp.name, sp.position, math::euler_to_quat(sp.rotation), sp.scale
                );
            }
        }

        return guard;
    }

    void setup_world_grid() {
        auto& world    = get_engine().get_world();
        auto& registry = world.get_model_registry();

        generator_params_ = {
            .voxel_scale = 16,
        };
        auto generator = std::make_unique<gfx::perlin_terrain_generator>(
            registry.get_identity_pool(), registry.get_page_pool(), generator_params_
        );
        world.set_world_grid(
            std::make_shared<gfx::world_grid<>>(
                world, std::move(generator), generator_params_.voxel_scale
            )
        );
    }

    void setup_player() {
        auto& world            = get_engine().get_world();
        auto& transform_system = world.get_transform_system();
        auto& physics_system   = world.get_physics_system();

        player_ = std::make_unique<gfx::entity_guard<>>(world);
        player_->with<gfx::hierarchy_component>();
        player_->with<gfx::transform_component>();
        player_->with<gfx::spatial_component>();
        player_->with<gfx::rigid_body_component>();
        player_->with<gfx::box_collider_component>();
        player_->with<gfx::character_controller_component>();
        player_->with<gfx::movement_intent_component>();
        player_->with<gfx::world_view_component>();
        player_->with<gfx::animation_player_component>();
        player_->with<gfx::animation_fsm_component>();

        const auto player_ent = player_->get_entity();

        transform_system.modify(player_ent)
            .set_position({0.0f, 500.0f, 0.0f})
            .set_origin({-6.0f, -6.0f, -6.0f});

        physics_system.modify_collider(player_ent)
            .set_extents({12.0f, 28.0f, 12.0f})
            .set_offset({0.0f, 2.0f, 0.0f});

        world.get_spatial_system().modify(player_ent).set_layer(gfx::spatial_layer::character);

        auto make_part = [&](std::string_view name) {
            return create_body_part(
                assets_.get_entity("m_human", name), assets_.get_model("m_human", name), player_ent
            );
        };

        body_       = make_part("body");
        head_       = make_part("head");
        hand_right_ = make_part("hand_right");
        hand_left_  = make_part("hand_left");
        foot_right_ = make_part("foot_right");
        foot_left_  = make_part("foot_left");

        setup_animation_fsm(player_ent);
    }

    void setup_animation_fsm(
        gfx::entity player_ent
    ) {
        auto& world = get_engine().get_world();
        auto& rb    = world.get_component<gfx::rigid_body_component>(player_ent);
        auto& move  = world.get_component<gfx::movement_intent_component>(player_ent);

        auto is_walk = [&move]() -> bool {
            const auto& v = move.get_wish_velocity();
            return v.x != 0.0f || v.z != 0.0f;
        };
        auto is_idle = [&move]() -> bool {
            const auto& v = move.get_wish_velocity();
            return v.x == 0.0f && v.z == 0.0f;
        };
        auto is_jump_left = [this, &rb]() -> bool {
            return jump_counter_ == 0 && !rb.is_grounded();
        };
        auto is_jump_right = [this, &rb]() -> bool {
            return jump_counter_ == 1 && !rb.is_grounded();
        };
        auto is_grounded = [&rb]() -> bool { return rb.is_grounded(); };

        constexpr gfx::transition blend_fast{.duration = 0.15f};
        constexpr gfx::transition blend_normal{.duration = 0.25f};
        constexpr gfx::transition blend_slow{.duration = 0.35f};

        gfx::animation_fsm fsm_movement;

        fsm_movement.add_state({
            .name      = "idle",
            .clip      = assets_.get_clip("a_idle"),
            .loop_mode = gfx::animation_loop_mode::loop,
            .transitions =
                {
                    {.target_state = "walk", .condition = is_walk, .blend = blend_fast},
                    {
                        .target_state     = "jump_left",
                        .condition        = is_jump_left,
                        .blend            = blend_fast,
                        .wait_until_blend = true,
                    },
                    {
                        .target_state     = "jump_right",
                        .condition        = is_jump_right,
                        .blend            = blend_fast,
                        .wait_until_blend = true,
                    },
                },
        });

        fsm_movement.add_state({
            .name           = "walk",
            .clip           = assets_.get_clip("a_walk"),
            .loop_mode      = gfx::animation_loop_mode::loop,
            .playback_speed = 1.25f,
            .transitions =
                {
                    {.target_state = "idle", .condition = is_idle, .blend = blend_fast},
                    {
                        .target_state     = "jump_left",
                        .condition        = is_jump_left,
                        .blend            = blend_fast,
                        .wait_until_blend = false,
                    },
                    {
                        .target_state     = "jump_right",
                        .condition        = is_jump_right,
                        .blend            = blend_fast,
                        .wait_until_blend = false,
                    },
                },
        });

        fsm_movement.add_state({
            .name           = "jump_right",
            .clip           = assets_.get_clip("a_jump_right"),
            .loop_mode      = gfx::animation_loop_mode::loop,
            .playback_speed = 1.0f,
            .transitions =
                {
                    {
                        .target_state     = "idle",
                        .condition        = is_grounded,
                        .blend            = blend_normal,
                        .wait_until_blend = true,
                    },
                },
        });

        fsm_movement.add_state({
            .name           = "jump_left",
            .clip           = assets_.get_clip("a_jump_left"),
            .loop_mode      = gfx::animation_loop_mode::loop,
            .playback_speed = 1.0f,
            .transitions =
                {
                    {
                        .target_state     = "idle",
                        .condition        = is_grounded,
                        .blend            = blend_normal,
                        .wait_until_blend = true,
                    },
                },
        });

        fsm_movement.set_entry_state("idle");

        world
            .get_animation_fsm_system()  //
            .modify(player_ent)
            .add_machine(0, std::move(fsm_movement));

        gfx::animation_fsm fsm_action;

        fsm_action.add_state({
            .name = "none",
            .transitions =
                {
                    {
                        .target_state = "sword_attack",
                        .trigger_name = "attack",
                        .blend        = blend_fast,
                    },
                },
        });

        fsm_action.add_state({
            .name            = "sword_attack",
            .clip            = assets_.get_clip("a_sword_attack"),
            .loop_mode       = gfx::animation_loop_mode::once,
            .layer_blend_in  = blend_normal,
            .layer_blend_out = blend_slow,
            .transitions =
                {
                    {
                        .target_state     = "none",
                        .condition        = []() -> bool { return true; },
                        .wait_until_end   = true,
                        .wait_until_blend = true,
                    },
                },
        });

        fsm_action.set_entry_state("none");

        world
            .get_animation_fsm_system()  //
            .modify(player_ent)
            .add_machine(1, std::move(fsm_action));
    }

    void try_place_player() {
        if (player_placed_) {
            return;
        }

        const auto grid    = get_engine().get_world().get_world_grid();
        const auto surface = grid->get_surface_y(0, 0);
        if (!surface) {
            return;
        }

        const auto voxel_scale = static_cast<float32>(generator_params_.voxel_scale);
        float32 spawn_y        = (static_cast<float32>(*surface) + 6.0f) * voxel_scale;

        auto& world = get_engine().get_world();
        world.get_transform_system()
            .modify(player_->get_entity())
            .set_position({0.0f, spawn_y, 0.0f});

        player_placed_ = true;
    }

    void render_ui() const {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos             = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags =          //
            ImGuiWindowFlags_NoCollapse |        //
            ImGuiWindowFlags_NoResize |          //
            ImGuiWindowFlags_NoScrollbar |       //
            ImGuiWindowFlags_AlwaysAutoResize |  //
            ImGuiWindowFlags_NoSavedSettings |   //
            ImGuiWindowFlags_NoNav |             //
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("Player Controller Test", nullptr, window_flags);
        ImGui::Text("Controls:");
        ImGui::Text("WASD - move");
        ImGui::Text("SPACE - jump");
        ImGui::Text("1 - toggle sword");
        ImGui::Text("LMB - sword attack");
        ImGui::Text("F - test impulse");
        ImGui::Text("Mouse - rotate camera");
        ImGui::Text("Scroll - zoom");
        ImGui::Text("F1 - toggle cursor");
        ImGui::Text("F2 - toggle colliders");
        ImGui::Text("ESC - exit");
        ImGui::Separator();

        if (player_) {
            auto& world           = get_engine().get_world();
            const auto player_ent = player_->get_entity();
            const auto& tc        = world.get_component<gfx::transform_component>(player_ent);
            const auto pos        = tc.get_position();
            ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

            const auto& rb = world.get_component<gfx::rigid_body_component>(player_ent);
            const auto vel = rb.get_velocity();
            ImGui::Text("Velocity: (%.1f, %.1f, %.1f)", vel.x, vel.y, vel.z);
            const auto imp = rb.get_impulse();
            ImGui::Text("Impulse: (%.1f, %.1f, %.1f)", imp.x, imp.y, imp.z);
            ImGui::Text("Grounded: %s", rb.is_grounded() ? "yes" : "no");
            ImGui::Text("Frozen: %s", rb.is_frozen() ? "yes" : "no");

            ImGui::Text("Arm length: %.1f", camera_controller_.get_actual_arm_length());

            ImGui::Separator();
            ImGui::Text("Animation FSM:");
            const auto& fsm_comp = world.get_component<gfx::animation_fsm_component>(player_ent);
            for (size_t i = 0; i < fsm_comp.machine_count(); ++i) {
                const auto& state = fsm_comp.get_machine(i).get_current_state();
                ImGui::Text("  Layer %zu: %s", i, state.c_str());
            }

            ImGui::Separator();
            ImGui::Text("Jump counter: %d", jump_counter_);
            ImGui::Text("Sword: %s", sword_ ? "equipped" : "none");
        }

        const auto grid = get_engine().get_world().get_world_grid();
        if (grid) {
            ImGui::Separator();
            ImGui::Text("Loaded chunks: %u", grid->get_loaded_chunk_count());
            ImGui::Text("Pending columns: %u", grid->get_pending_column_count());
        }

        const auto& stats = get_engine().get_world().get_update_stats();
        ImGui::Separator();
        ImGui::Text("Character Controller: %.2f ms", stats.character_controller_ms);
        ImGui::Text("Physics: %.2f ms", stats.physics_ms);

        ImGui::End();
    }

    void toggle_sword() {
        if (!hand_right_) {
            return;
        }

        auto& world         = get_engine().get_world();
        const auto hand_ent = hand_right_->get_entity();

        if (sword_) {
            world
                .get_socket_system()  //
                .modify(hand_ent)
                .detach("hand_right");
            sword_ = nullptr;
        } else {
            sword_ = std::make_unique<gfx::entity_guard<>>(world);
            sword_->with<gfx::hierarchy_component>();
            sword_->with<gfx::transform_component>();
            sword_->with<gfx::spatial_component>();
            sword_->with<gfx::model_component>();

            const auto sword_ent = sword_->get_entity();
            const auto& ent_data = assets_.get_entity("m_sword", "root");
            world
                .get_transform_system()  //
                .modify(sword_ent)
                .set_origin(ent_data.origin);
            world
                .get_model_system()  //
                .modify(sword_ent)
                .set_model(assets_.get_model("m_sword", "root"));
            world
                .get_socket_system()  //
                .modify(hand_ent)
                .attach("hand_right", sword_ent);
            world
                .get_spatial_system()  //
                .modify(sword_ent)
                .set_layer(gfx::spatial_layer::character);
        }
    }

    void handle_mouse_press(
        gfx::mouse::buttons button
    ) {
        switch (button) {
            case gfx::mouse::buttons::LEFT:
                if (player_ && sword_) {
                    const auto player_ent = player_->get_entity();

                    auto& world = get_engine().get_world();
                    const auto& anim_player =
                        world.get_component<gfx::animation_player_component>(player_ent);

                    const auto& layer = anim_player.get_layer(1);
                    if (!layer.is_active()) {
                        world  //
                            .get_animation_fsm_system()
                            .modify(player_ent)
                            .fire_trigger("attack");
                    }
                }
                break;
            default:
                break;
        }
    }

    void handle_key_press(
        gfx::keyboard::keys key
    ) {
        switch (key) {
            case gfx::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            case gfx::keyboard::keys::KEY_1:
                toggle_sword();
                break;
            case gfx::keyboard::keys::F:
                if (player_) {
                    get_engine()
                        .get_world()
                        .get_physics_system()
                        .modify(player_->get_entity())
                        .add_external_impulse({0.0f, 0.0f, 500.0f});
                }
                break;
            case gfx::keyboard::keys::F1:
                input_controller_.set_mouse_captured(!input_controller_.is_mouse_captured());
                break;
            case gfx::keyboard::keys::F2:
                show_colliders_ = !show_colliders_;
                break;
            default:
                break;
        }
    }

    gfx::vox_parser_plain parser_;
    gfx::asset_storage assets_;

    gfx::player_input_controller input_controller_;
    gfx::third_person_camera_controller<> camera_controller_;
    gfx::third_person_player_controller<> player_controller_;

    std::unique_ptr<gfx::entity_guard<>> player_;
    std::unique_ptr<gfx::entity_guard<>> body_;
    std::unique_ptr<gfx::entity_guard<>> head_;
    std::unique_ptr<gfx::entity_guard<>> hand_right_;
    std::unique_ptr<gfx::entity_guard<>> hand_left_;
    std::unique_ptr<gfx::entity_guard<>> foot_right_;
    std::unique_ptr<gfx::entity_guard<>> foot_left_;
    std::unique_ptr<gfx::entity_guard<>> sword_;

    gfx::perlin_terrain_generator::params generator_params_;
    bool player_placed_  = false;
    bool show_colliders_ = true;

    bool need_update_jump_ = false;
    int jump_counter_      = 0;
};

auto main() -> int {
    try {
        log::logger::get().add_file_sink("test_player_controller.log");
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - Player Controller Test")
            ->run<player_controller_app>();
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
