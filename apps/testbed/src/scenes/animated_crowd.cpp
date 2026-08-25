module;

#include <imgui.h>

module vw.testbed;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.gfx;

namespace vw::testbed {

animated_crowd_scene::animated_crowd_scene(
    testbed_app& stand, const arg_reader& args
)
    : scene{stand}, size_{args.count("--bodies", 50)} {}

auto animated_crowd_scene::on_world_ready() -> void {
    if (spawned_ || size_ == 0) {
        return;
    }

    spawn_(stand().altitude());
    spawned_ = true;
}

auto animated_crowd_scene::tick(float32 /*delta_time*/) -> void {
    if (bodies_.empty()) {
        return;
    }
    if (settle_frames_ < settle_target) {
        ++settle_frames_;
    }
}

auto animated_crowd_scene::make_clip_(
    ecs::world& world
) -> std::shared_ptr<asset::animation_clip> {
    auto& clips = world.resource<asset::animation_clip_registry>();
    auto clip   = clips.create("crowd_wave");

    for (std::size_t part = 0; part < target_names.size(); ++part) {
        asset::animation_track track{std::string{target_names[part]}, 60.0f};

        asset::animation_channel<vec3f> channel{asset::animation_property::position};
        const float32 phase = static_cast<float32>(part) * 0.25f;
        channel.add(asset::keyframe_vec3f{0.0f, vec3f{0.0f, phase, 0.0f}});
        channel.add(asset::keyframe_vec3f{0.5f, vec3f{0.0f, phase + 2.0f, 0.0f}});
        channel.add(asset::keyframe_vec3f{1.0f, vec3f{0.0f, phase, 0.0f}});
        track.add<asset::animation_property::position>(std::move(channel));

        clip->add_track(std::move(track));
    }

    return clip;
}

auto animated_crowd_scene::spawn_(float32 ground_y) -> void {
    auto& world    = stand().world();
    auto& registry = world.resource<asset::model_registry>();

    auto body = registry.create("crowd_body", 6, 12, 4);
    body->fill(voxel{blocks::blue_3});
    auto head = registry.create("crowd_head", 6, 6, 6);
    head->fill(voxel{blocks::brown_2});
    auto hand = registry.create("crowd_hand", 3, 8, 3);
    hand->fill(voxel{blocks::green_4});

    const std::array<std::pair<std::shared_ptr<asset::model>, vec3f>, 4> parts{{
        {body, vec3f{0.0f, 8.0f, 0.0f}},
        {head, vec3f{0.0f, 20.0f, 0.0f}},
        {hand, vec3f{-5.0f, 8.0f, 0.0f}},
        {hand, vec3f{5.0f, 8.0f, 0.0f}},
    }};

    const auto clip = make_clip_(world);

    const auto side =
        static_cast<int32>(std::ceil(std::sqrt(static_cast<float32>(size_))));
    constexpr float32 spacing = 40.0f;
    const float32 origin      = -0.5f * static_cast<float32>(side - 1) * spacing;

    auto& transform_sys = world.system<ecs::transform_system>();
    auto& hierarchy_sys = world.system<ecs::hierarchy_system>();
    auto& physics_sys   = world.system<ecs::physics_system>();
    auto& model_sys     = world.system<ecs::model_system>();
    auto& anim_sys      = world.system<ecs::animation_system>();

    for (uint32 i = 0; i < size_; ++i) {
        const auto col = static_cast<int32>(i) % side;
        const auto row = static_cast<int32>(i) / side;

        const auto root = world.create()
                              .with<ecs::hierarchy_component>()
                              .with<ecs::transform_component>()
                              .with<ecs::spatial_component>()
                              .with<ecs::rigid_body_component>()
                              .with<ecs::box_collider_component>()
                              .with<ecs::animation_player_component>()
                              // Радиус от собственной ширины тела: коллайдер
                              // двенадцать поперёк, значит диск чуть шире стоп.
                              // Высота падения — одно тело: прыжок выше неё
                              // ополовинивает пятно.
                              .with(ecs::blob_shadow_component{8.0f, 48.0f, 0.55f})
                              .get_entity();

        // Сбрасываются чуть выше земли и получают время приземлиться до начала
        // прогона: посадка руками на неровный рельеф оставляет тела частично
        // вкопанными, а выталкивание хуже падения.
        transform_sys.modify(root).set_position({
            origin + (static_cast<float32>(col) * spacing),
            ground_y + 40.0f,
            origin + (static_cast<float32>(row) * spacing),
        });
        physics_sys.modify_collider(root).set_extents({12.0f, 24.0f, 12.0f});
        world.system<ecs::spatial_system>().modify(root).set_layer(
            ecs::spatial_layer::character
        );

        for (std::size_t part = 0; part < parts.size(); ++part) {
            const auto ent = world.create()
                                 .with<ecs::hierarchy_component>()
                                 .with<ecs::transform_component>()
                                 .with<ecs::spatial_component>()
                                 .with<ecs::model_component>()
                                 .with<ecs::animation_target_component>()
                                 .get_entity();

            hierarchy_sys.modify(ent).set_parent(root);

            transform rest;
            rest.set_position(parts[part].second);
            transform_sys.modify(ent).set_transform(rest);
            model_sys.modify(ent).set_model(parts[part].first);

            const auto target = anim_sys.modify_target(ent);
            target.set_target_name(std::string{target_names[part]});
            target.set_rest_transform(rest);

            bodies_.push_back(ent);
        }

        auto player = anim_sys.modify_player(root);
        player.add_layer(0);
        player.layer(0).blend_to(clip);
        player.layer(0).set_loop_mode(asset::animation_loop_mode::loop);
        player.layer(0).play();

        bodies_.push_back(root);
    }

    log::info("animated-crowd: {} bodies, {} entities", size_, bodies_.size());
}

auto animated_crowd_scene::ui() -> void {
    ImGui::Text("animated-crowd: %u bodies, %zu entities, settled %u/%u", size_, bodies_.size(),
                settle_frames_, settle_target);
}

}  // namespace vw::testbed
