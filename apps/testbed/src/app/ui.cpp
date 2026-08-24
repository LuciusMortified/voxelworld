module;

#include <imgui.h>

module vw.testbed;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::testbed {

auto testbed_app::render_ui() -> void {
    // Захваченный курсор всё равно двигает указатель ImGui, поэтому резкий
    // поворот камеры сажает его на кнопку, и панель загорается под прицелом,
    // которого там нет. NoMouse забирает указатель у ImGui вовсе, пока камера
    // им владеет.
    ImGuiIO& io = ImGui::GetIO();
    if (camera_controller_->is_mouse_captured()) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos             = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::Begin("World Grid Test", nullptr, window_flags);
    ImGui::Text("Controls:");
    ImGui::Text("WASD + Mouse - moving");
    ImGui::Text("F1 - toggle cursor");
    ImGui::Text("LMB - use the tool, cursor captured");
    ImGui::Text("N - pause the sun, [ ] - move it");
    ImGui::Text("ESC - exit");
    ImGui::Separator();

    {
        const auto hour = static_cast<int32>(time_of_day_ * 24.0f);
        const auto minute =
            static_cast<int32>(((time_of_day_ * 24.0f) - static_cast<float32>(hour)) * 60.0f);
        ImGui::Text("%02d:%02d %s", hour, minute, day_night_running_ ? "" : "(paused)");

        float32 time = time_of_day_;
        if (ImGui::SliderFloat("Time", &time, 0.0f, 1.0f, "%.3f")) {
            time_of_day_ = time;
            apply_time_of_day_();
        }
        ImGui::SliderFloat("Day (s)", &day_length_seconds_, 8.0f, 600.0f, "%.0f");
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Editing")) {
        static constexpr std::array<const char*, 3> tool_names{"none", "place", "remove"};

        auto tool = static_cast<int32>(tool_);
        if (ImGui::Combo(
                "Tool", &tool, tool_names.data(), static_cast<int32>(tool_names.size())
            )) {
            tool_ = static_cast<edit_tool>(tool);
        }

        if (tool_ == edit_tool::place) {
            std::array<const char*, block_menu.size()> names{};
            for (std::size_t i = 0; i < block_menu.size(); ++i) {
                names[i] = block_menu[i].name;
            }
            ImGui::Combo("Block", &place_choice_, names.data(), static_cast<int32>(names.size()));
        }

        ImGui::SliderInt("Reach (voxels)", &reach_voxels_, 2, 32);

        if (tool_ == edit_tool::none) {
            ImGui::TextUnformatted("pick a tool, capture the cursor with F1, left click");
        } else if (!camera_controller_->is_mouse_captured()) {
            ImGui::TextUnformatted("F1 to capture the cursor");
        } else if (hovered_) {
            ImGui::Text(
                "voxel %d,%d,%d", hovered_->solid.x, hovered_->solid.y, hovered_->solid.z
            );
        } else {
            ImGui::TextUnformatted("nothing in reach");
        }

        ImGui::Text("edits: %d", edit_clicks_);

        // Что показать про содержимое сцены, знает только сама сцена.
        if (scene_ != nullptr) {
            scene_->ui();
        }
    }

    ImGui::Separator();

    // No shadow panel: the cascades are parked, and sliders that move
    // nothing are worse than none. Turning them back on is two edits --
    // shadow_settings::enabled and SHADOW_ENABLED in voxel.frag.
    if (ImGui::CollapsingHeader("Lighting")) {
        auto& renderer = get_engine().get_renderer();
        auto& ambient  = renderer.get_ambient_settings();

        ImGui::SliderFloat("Ambient", &ambient.strength, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("AO strength", &ambient.ao_strength, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("AO curve", &ambient.ao_curve, 0.25f, 4.0f, "%.2f");

        // Drag this to zero and look at a hillside from the downhill side:
        // the steps flatten into one plane, because occlusion has nothing
        // to say about a corner with nothing above it.
        ImGui::SliderFloat("Convex strength", &ambient.convex_strength, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Convex curve", &ambient.convex_curve, 0.25f, 4.0f, "%.2f");

        ImGui::ColorEdit3("Cave ambient", &ambient.cave.x);

        // Exposure moves the whole picture; the white point decides where
        // the roll-off lands. Drop the white point under what the brightest
        // face reaches and that face clips again, which is the state this
        // replaced.
        auto& tonemap = renderer.get_tonemap_settings();
        ImGui::SliderFloat("Exposure", &tonemap.exposure, 0.1f, 4.0f, "%.2f");
        ImGui::SliderFloat("White point", &tonemap.white_point, 0.25f, 4.0f, "%.2f");

        // Zero is plain Lambert. Set the time to noon and drag it there:
        // three of the four walls of every voxel collapse onto one colour,
        // because the sun gives all three exactly nothing.
        auto& sun = renderer.get_directional_light_settings();
        ImGui::SliderFloat("Sun wrap", &sun.wrap, 0.0f, 1.0f, "%.2f");

        ImGui::SliderFloat("Sky curve", &ambient.sky_curve, 0.25f, 4.0f, "%.2f");

        // Drag this to one and stand in a cave mouth: daylight walks
        // fifteen voxels in, because that is how far the flood carries it.
        // Sky light is the sun's only occluder now, so this is the whole
        // say over how sharply the sun stops at an opening.
        ImGui::SliderFloat("Sun curve", &ambient.sun_curve, 0.25f, 8.0f, "%.2f");

        // One colour for every light block there is. Set the time to
        // midnight and it is the only thing still lighting anything --
        // which is the point of keeping this channel out of the day.
        auto& lamp = renderer.get_block_light_settings();
        ImGui::ColorEdit3("Lamp colour", &lamp.color.x);
        ImGui::SliderFloat("Lamp strength", &lamp.intensity, 0.0f, 4.0f, "%.2f");

        // At one the fifteen baked steps come out even and read as a ramp
        // painted on the wall. Two is near the curve Minecraft's lightmap
        // uses and reads as falloff.
        ImGui::SliderFloat("Lamp curve", &lamp.curve, 0.25f, 4.0f, "%.2f");

        // At one a lava face comes out as exactly the colour lava was
        // drawn where nothing else reaches it. That is the anchor; above
        // it the tone curve starts taking the difference back.
        ImGui::SliderFloat("Glow", &lamp.glow, 0.0f, 3.0f, "%.2f");

        // The dynamic half of the same light. Light it, place a lamp, and
        // walk one onto the other: if the falloffs have drifted apart this
        // is where it shows.
        bool torch = torch_.is_valid();
        if (ImGui::Checkbox("Carry a torch", &torch)) {
            set_torch_(torch);
        }

        // Zero is the honest comparison: the crowd with nothing under it,
        // which is what the patch is meant to fix.
        ImGui::SliderFloat(
            "Blob shadow", &get_engine().get_renderer().get_blob_strength(), 0.0f, 1.0f,
            "%.2f"
        );

        // Nothing in the terrain emits, so without these there is nothing
        // to look at. Both write through world_grid::set_voxel, which is
        // the same path an edit takes -- the column goes dirty, the baker
        // floods it again and the chunks whose light moved are meshed
        // again. Watching the relight counters below tick is half the
        // point of the buttons.
        if (world_grid_ != nullptr) {
            if (ImGui::Button("Drop lamp")) {
                drop_emitter(blocks::lamp, 1);
            }
            ImGui::SameLine();
            if (ImGui::Button("Pour lava")) {
                drop_emitter(blocks::lava, 3);
            }

            // A button that does nothing and says nothing is the worst of
            // both: the first version of this missed the ground by a factor
            // of the voxel scale and looked exactly like a broken shader.
            if (!drop_status_.empty()) {
                ImGui::TextUnformatted(drop_status_.c_str());
            }
        }

        // Judging occlusion off the finished frame means judging a product
        // of the block's colour and everything falling on it. These show
        // one factor with the others taken away.
        static constexpr std::array<const char*, 9> view_names{
            "off",         "ambient occlusion", "normals",
            "sky light",   "convexity",         "block light",
            "blob shadow", "light complexity",  "blob complexity"
        };
        auto view = static_cast<int32>(renderer.get_debug_view());
        if (ImGui::Combo(
                "Debug view", &view, view_names.data(), static_cast<int32>(view_names.size())
            )) {
            renderer.set_debug_view(static_cast<gfx::debug_view>(view));
        }

        // The switch the acceptance of the froxel pass rests on: the same
        // frame lit both ways, with nothing else moving between the two.
        auto& clusters = renderer.get_cluster_settings();
        ImGui::Checkbox("Clustered lights", &clusters.enabled);
        ImGui::SameLine();
        ImGui::Text("%u x %u x %u, cap %u",
            renderer.get_cluster_grid(get_engine().get_camera()).tiles_x(),
            renderer.get_cluster_grid(get_engine().get_camera()).tiles_y(),
            clusters.slices, clusters.cap);

        ImGui::Text(
            "sky   %.3f %.3f %.3f", ambient.sky.x, ambient.sky.y, ambient.sky.z
        );
        ImGui::Text(
            "grnd  %.3f %.3f %.3f", ambient.ground.x, ambient.ground.y, ambient.ground.z
        );
    }

    ImGui::Separator();

    const auto& camera = get_engine().get_camera();
    const auto pos     = camera.get_position();
    ImGui::Text("Camera: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

    if (world_grid_) {
        auto chunk_coord = world_grid_->world_to_chunk_coord(
            {static_cast<int32>(pos.x), static_cast<int32>(pos.y), static_cast<int32>(pos.z)}
        );
        ImGui::Text("Chunk: (%d, %d, %d)", chunk_coord.x, chunk_coord.y, chunk_coord.z);
        const auto& wgs = get_engine().get_world().system<ecs::world_grid_system>();
        ImGui::Text("Loaded columns: %u", world_grid_->column_count());
        ImGui::Text(
            "Loaded chunks: %u (%u drawn)", world_grid_->chunk_count(),
            world_grid_->drawn_chunk_count()
        );
        ImGui::Text("Pending columns: %u", wgs.get_stats().pending_count);
        ImGui::Text(
            "Pending meshes: %u", get_engine().get_renderer().get_mesh_pool().get_pending_count()
        );
    }

    ImGui::Separator();
    float speed = camera_controller_->get_camera_speed();
    if (ImGui::SliderFloat("Speed", &speed, 1.0f, 5000.0f, "%.0f")) {
        camera_controller_->set_camera_speed(speed);
    }

    ImGui::End();
}

auto testbed_app::handle_key_press(
    plat::keyboard::keys key
) -> void {
    switch (key) {
        case plat::keyboard::keys::ESCAPE:
            get_engine().shutdown();
            break;
        case plat::keyboard::keys::F1:
            camera_controller_->toggle_mouse_captured();
            camera_controller_->toggle_keyboard_control_enabled();
            break;
        case plat::keyboard::keys::N:
            day_night_running_ = !day_night_running_;
            break;
        case plat::keyboard::keys::LEFT_BRACKET:
            step_time_of_day_(-0.02f);
            break;
        case plat::keyboard::keys::RIGHT_BRACKET:
            step_time_of_day_(0.02f);
            break;
        default:
            break;
    }
}

}  // namespace vw::testbed
