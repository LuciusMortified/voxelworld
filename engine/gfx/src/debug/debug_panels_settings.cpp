module;

#include <imgui.h>

module vw.gfx;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;

namespace vw::gfx {

namespace {

// Ползунки настроек стоят узкими намеренно: панель раскрывается деревьями, и
// окно, ширину которого задаёт самый длинный ползунок, перекрывает сцену.
constexpr float32 item_width = 170.0f;

// Сколько источников оставить, когда предел включают руками. По умолчанию предела
// нет вовсе, и ползунку нужно с чего-то начинать.
constexpr uint32 default_visible_cap = 256;

auto reset_button(const char* id) -> bool {
    ImGui::Spacing();
    return ImGui::Button(id);
}

auto drag_uint(const char* label, uint32& value, uint32 low, uint32 high) -> void {
    auto shown = static_cast<int32>(value);
    if (ImGui::SliderInt(label, &shown, static_cast<int32>(low), static_cast<int32>(high))) {
        value = static_cast<uint32>(shown);
    }
}

}  // namespace

auto debug_window::render_view_panel() -> void {
    auto& renderer = engine_->get_renderer();

    ImGui::TextUnformatted("mode");
    for (std::size_t i = 0; i < render_mode_names.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
        }
        const bool active = static_cast<std::size_t>(renderer.get_render_mode()) == i;
        if (ImGui::RadioButton(render_mode_names[i].data(), active)) {
            renderer.set_render_mode(static_cast<render_mode>(i));
        }
    }

    ImGui::PushItemWidth(item_width);

    // Каждый режим показывает один множитель освещения отдельно — судить об
    // одном из них по готовому кадру значит судить по произведению.
    const auto current = static_cast<std::size_t>(renderer.get_debug_view());
    if (ImGui::BeginCombo("debug view", debug_view_names[current].data())) {
        for (std::size_t i = 0; i < debug_view_names.size(); ++i) {
            const bool selected = i == current;
            if (ImGui::Selectable(debug_view_names[i].data(), selected)) {
                renderer.set_debug_view(static_cast<debug_view>(i));
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    ImGui::Checkbox("colliders", &show_colliders_);

    // Обход идёт в обоих положениях, поэтому это честное сравнение одной сборки
    // с собой же; что он скрыл, видно в окне World.
    bool chunk_cull = renderer.is_chunk_cull_enabled();
    if (ImGui::Checkbox("chunk cull", &chunk_cull)) {
        renderer.set_chunk_cull_enabled(chunk_cull);
    }
}

auto debug_window::render_lighting_panel() -> void {
    auto& renderer = engine_->get_renderer();
    ImGui::PushItemWidth(item_width);

    if (ImGui::TreeNode("Sun")) {
        auto& sun = renderer.get_directional_light_settings();

        // Приложение вправе вести солнце само — цикл дня перепишет это на
        // следующем же кадре, и ползунок вернётся туда, где был.
        ImGui::TextDisabled("the app may drive these every frame");

        if (ImGui::DragFloat3("direction", &sun.direction.x, 0.01f, -1.0f, 1.0f, "%.2f")) {
            // Шейдер ждёт единичный вектор: несвёрнутое направление — это
            // умножение всего дневного света на его длину.
            if (math::length(sun.direction) > 0.0f) {
                sun.direction = math::normalize(sun.direction);
            }
        }
        ImGui::ColorEdit3("colour", &sun.color.x);
        ImGui::SliderFloat("intensity", &sun.intensity, 0.0f, 4.0f, "%.2f");

        // Ноль — обычный ламберт: в полдень три вертикальные грани из четырёх
        // получают ровно ничего и сходятся в один цвет.
        ImGui::SliderFloat("wrap", &sun.wrap, 0.0f, 1.0f, "%.2f");

        if (reset_button("reset##sun")) {
            sun = directional_light_settings{};
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Ambient")) {
        auto& ambient = renderer.get_ambient_settings();

        ImGui::SliderFloat("strength", &ambient.strength, 0.0f, 2.0f, "%.2f");
        ImGui::ColorEdit3("sky", &ambient.sky.x);
        ImGui::ColorEdit3("ground", &ambient.ground.x);
        ImGui::ColorEdit3("cave", &ambient.cave.x);

        ImGui::SliderFloat("ao strength", &ambient.ao_strength, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("ao curve", &ambient.ao_curve, 0.25f, 4.0f, "%.2f");

        // На нуле склон холма со стороны обрыва схлопывается в одну плоскость:
        // затенению нечего сказать про угол, над которым ничего нет.
        ImGui::SliderFloat("convex strength", &ambient.convex_strength, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("convex curve", &ambient.convex_curve, 0.25f, 4.0f, "%.2f");

        ImGui::SliderFloat("sky curve", &ambient.sky_curve, 0.25f, 4.0f, "%.2f");

        // На единице дневной свет заходит в устье пещеры на пятнадцать вокселей:
        // небесный свет — единственный окклюдер солнца.
        ImGui::SliderFloat("sun curve", &ambient.sun_curve, 0.25f, 8.0f, "%.2f");

        if (reset_button("reset##ambient")) {
            ambient = ambient_settings{};
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Block light")) {
        auto& lamp = renderer.get_block_light_settings();

        ImGui::ColorEdit3("colour", &lamp.color.x);
        ImGui::SliderFloat("intensity", &lamp.intensity, 0.0f, 4.0f, "%.2f");

        // На единице пятнадцать запечённых ступеней выходят ровными и читаются
        // как нарисованный на стене скат; двойка — кривая лайтмапа Minecraft.
        ImGui::SliderFloat("curve", &lamp.curve, 0.25f, 4.0f, "%.2f");

        // На единице грань лавы выходит ровно того цвета, каким лава нарисована,
        // там, где её ничто больше не освещает. Это якорь, а не вкус.
        ImGui::SliderFloat("glow", &lamp.glow, 0.0f, 3.0f, "%.2f");

        if (reset_button("reset##lamp")) {
            lamp = block_light_settings{};
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Blob shadows")) {
        // Ноль — честное сравнение: тела с ничем под ними, то есть то, что
        // пятна и должны были поправить.
        ImGui::SliderFloat("strength", &renderer.get_blob_strength(), 0.0f, 1.0f, "%.2f");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Tonemap")) {
        auto& tonemap = renderer.get_tonemap_settings();

        // Экспозиция двигает картинку целиком, белая точка решает, где встанет
        // скат: опусти её ниже яркости самой светлой грани — и та снова срежется.
        ImGui::SliderFloat("exposure", &tonemap.exposure, 0.1f, 4.0f, "%.2f");
        ImGui::SliderFloat("white point", &tonemap.white_point, 0.25f, 4.0f, "%.2f");

        if (reset_button("reset##tonemap")) {
            tonemap = tonemap_settings{};
        }
        ImGui::TreePop();
    }

    ImGui::PopItemWidth();
}

auto debug_window::render_shadows_panel() -> void {
    auto& shadows = engine_->get_renderer().get_shadow_settings();
    ImGui::PushItemWidth(item_width);

    ImGui::Checkbox("enabled", &shadows.enabled);

    // Парный выключатель живёт в шейдере, и они обязаны совпадать: включённый
    // здесь и выключенный там даёт проход, который рисует и ничего не меняет.
    ImGui::TextDisabled("twin switch: SHADOW_ENABLED in voxel.frag");

    ImGui::SliderFloat("first split", &shadows.first_split, 4.0f, 256.0f, "%.0f");
    ImGui::SliderFloat("distance", &shadows.distance, 100.0f, 4000.0f, "%.0f");
    ImGui::SliderFloat("turn texels", &shadows.turn_texels, 0.0f, 2.0f, "%.3f");
    drag_uint("updates/frame", shadows.updates_per_frame, 0, shadow_map::cascade_count);
    ImGui::SliderFloat("filter texels", &shadows.filter_texels, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat("normal bias", &shadows.normal_bias, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat("slope bias", &shadows.slope_bias, 0.0f, 4.0f, "%.2f");

    // Тексель каскада — то, чем меряется лесенка на его границе, поэтому вынос
    // первого разбиения дальше размывает ближнюю зону, а не удлиняет её.
    ImGui::SeparatorText("cascades");
    const auto& splits = engine_->get_renderer().get_cascade_splits();
    const auto& texels = engine_->get_renderer().get_cascade_texel_sizes();
    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        ImGui::Text("%u: to %8.1f, texel %6.3f", i, splits[i], texels[i]);
    }

    if (reset_button("reset##shadows")) {
        shadows = shadow_settings{};
    }

    ImGui::PopItemWidth();
}

auto debug_window::render_lights_panel() -> void {
    auto& renderer = engine_->get_renderer();
    auto& clusters = renderer.get_cluster_settings();
    ImGui::PushItemWidth(item_width);

    // Выключатель — не вкус: с выключенной сеткой фрагмент обходит все источники
    // кадра, и только так «картинка не изменилась» остаётся проверяемым.
    ImGui::Checkbox("clustered", &clusters.enabled);

    drag_uint("tile size", clusters.tile_size, 8, 128);
    drag_uint("slices", clusters.slices, 1, 64);
    drag_uint("light cap", clusters.cap, 1, 256);
    drag_uint("blob cap", clusters.blob_cap, 1, 64);

    const auto grid = renderer.get_cluster_grid(engine_->get_camera());
    ImGui::Text(
        "%u x %u x %u, %u clusters", grid.tiles_x(), grid.tiles_y(), clusters.slices,
        grid.cluster_count()
    );

    // Столько источников фрагментный шейдер обходит на каждый пиксель — и это
    // единственный способ отличить нагруженную сцену от сцены, чьи источники
    // все за камерой.
    ImGui::Text("visible lights %u", renderer.get_visible_light_count());

    auto& max_visible = renderer.get_max_visible_lights();
    bool capped       = max_visible != light_buffer::no_cap;
    if (ImGui::Checkbox("cap visible", &capped)) {
        max_visible = capped ? default_visible_cap : light_buffer::no_cap;
    }
    if (capped) {
        drag_uint("max visible", max_visible, 1, 4096);
    }

    ImGui::PopItemWidth();
}

auto debug_window::render_fog_panel() -> void {
    auto& fog = engine_->get_renderer().get_fog_settings();
    ImGui::PushItemWidth(item_width);

    ImGui::Checkbox("enabled", &fog.enabled);
    ImGui::ColorEdit3("colour", &fog.color.x);
    ImGui::SliderFloat("near", &fog.near_distance, 0.0f, 4096.0f, "%.0f");
    ImGui::SliderFloat("far", &fog.far_distance, 0.0f, 8192.0f, "%.0f");

    if (reset_button("reset##fog")) {
        fog = fog_settings{};
    }

    ImGui::PopItemWidth();
}

}  // namespace vw::gfx
