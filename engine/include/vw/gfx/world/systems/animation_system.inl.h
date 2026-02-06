#pragma once

#include <cmath>
#include <vector>

namespace vw::gfx {

template <typename... Cs>
animation_system<Cs...>::animation_system(
    world_type& world,
    registry_type& registry,
    transform_system_type& transform_sys,
    animation_clip_registry& clip_registry
)
    : world_(&world),
      registry_(&registry),
      transform_system_(&transform_sys),
      clip_registry_(&clip_registry) {}

template <typename... Cs>
void animation_system<Cs...>::update(float32 delta_time) {
    std::vector<entity> to_remove;

    for (entity ent : active_entities_) {
        if (!registry_->template has<animation_component>(ent)) {
            to_remove.push_back(ent);
            continue;
        }

        auto& anim_comp = registry_->template get<animation_component>(ent);

        process_animation(ent, anim_comp, delta_time);

        if (anim_comp.state_ == animation_state::stopped) {
            to_remove.push_back(ent);
        }
    }

    for (entity ent : to_remove) {
        remove_active_entity(ent);
    }
}

template <typename... Cs>
void animation_system<Cs...>::add_active_entity(entity root_ent) {
    active_entities_.insert(root_ent);

    build_and_cache_target_map(root_ent);
}

template <typename... Cs>
void animation_system<Cs...>::remove_active_entity(entity root_ent) {
    active_entities_.erase(root_ent);

    target_maps_.erase(root_ent);
}

template <typename... Cs>
void animation_system<Cs...>::build_and_cache_target_map(entity root_ent) {
    std::unordered_map<std::string, entity> target_map;

    std::queue<entity> to_visit;
    to_visit.push(root_ent);

    while (!to_visit.empty()) {
        entity current = to_visit.front();
        to_visit.pop();

        if (registry_->template has<animation_target_component>(current)) {
            const auto& target_comp = registry_->template get<animation_target_component>(current);
            target_map[target_comp.get_name()] = current;
        }

        if (registry_->template has<hierarchy_component>(current)) {
            const auto& hierarchy = registry_->template get<hierarchy_component>(current);
            for (entity child : hierarchy.get_children()) {
                to_visit.push(child);
            }
        }
    }

    target_maps_[root_ent] = std::move(target_map);
}

template <typename... Cs>
auto animation_system<Cs...>::get_cached_target_map(entity root_ent) const
    -> const std::unordered_map<std::string, entity>* {
    auto it = target_maps_.find(root_ent);
    if (it != target_maps_.end()) {
        return &it->second;
    }
    return nullptr;
}

template <typename... Cs>
void animation_system<Cs...>::process_animation(
    entity ent,
    animation_component& anim_comp,
    float32 delta_time
) {
    if (!anim_comp.clip_) {
        return;
    }

    if (anim_comp.state_ != animation_state::playing) {
        return;
    }

    anim_comp.current_time_ += delta_time * anim_comp.playback_speed_ * anim_comp.direction_;

    float32 duration = anim_comp.clip_->get_duration();

    if (anim_comp.loop_mode_ == animation_loop_mode::once) {
        if (anim_comp.current_time_ >= duration) {
            anim_comp.current_time_ = duration;
            anim_comp.state_ = animation_state::stopped;
        } else if (anim_comp.current_time_ < 0.0f) {
            anim_comp.current_time_ = 0.0f;
            anim_comp.state_ = animation_state::stopped;
        }
    } else if (anim_comp.loop_mode_ == animation_loop_mode::loop) {
        if (duration > 0.0f) {
            anim_comp.current_time_ = std::fmod(anim_comp.current_time_, duration);
            if (anim_comp.current_time_ < 0.0f) {
                anim_comp.current_time_ += duration;
            }
        }
    } else if (anim_comp.loop_mode_ == animation_loop_mode::ping_pong) {
        if (anim_comp.current_time_ >= duration) {
            anim_comp.direction_ = -1.0f;
            anim_comp.current_time_ = duration;
        } else if (anim_comp.current_time_ <= 0.0f) {
            anim_comp.direction_ = 1.0f;
            anim_comp.current_time_ = 0.0f;
        }
    }

    if (anim_comp.blend_duration_ > 0.0f) {
        anim_comp.blend_time_ += delta_time;
        if (anim_comp.blend_time_ >= anim_comp.blend_duration_) {
            anim_comp.previous_clip_ = nullptr;
            anim_comp.blend_time_ = 0.0f;
            anim_comp.blend_duration_ = 0.0f;
        }
    }

    apply_animation_to_transform(ent, anim_comp);
}

template <typename... Cs>
void animation_system<Cs...>::apply_animation_to_transform(
    entity root_ent,
    const animation_component& anim_comp
) {
    if (!anim_comp.clip_) {
        return;
    }

    const auto* target_map = get_cached_target_map(root_ent);
    if (!target_map) {
        return;
    }

    auto get_blended_transform = [&](const animation_track& track, float32 time) -> transform {
        transform current = track.evaluate(time);

        if (anim_comp.previous_clip_ && anim_comp.blend_duration_ > 0.0f) {
            auto* prev_track = anim_comp.previous_clip_->get_track(track.target_name);
            if (prev_track) {
                transform previous = prev_track->evaluate(anim_comp.previous_time_);
                float32 blend_factor = anim_comp.blend_time_ / anim_comp.blend_duration_;
                current = blend_transforms(previous, current, blend_factor);
            }
        }

        return current;
    };

    for (const auto& track : anim_comp.clip_->get_tracks()) {
        auto it = target_map->find(track.target_name);
        if (it == target_map->end()) {
            continue;
        }

        entity target_ent = it->second;

        if (!registry_->template has<transform_component>(target_ent)) {
            continue;
        }

        transform t = get_blended_transform(track, anim_comp.current_time_);
        auto modifier = transform_system_->modify(target_ent);
        modifier.set_transform(t);
    }
}

template <typename... Cs>
auto animation_system<Cs...>::blend_transforms(
    const transform& t1,
    const transform& t2,
    float32 factor
) const -> transform {
    factor = std::clamp(factor, 0.0f, 1.0f);

    transform result;

    vec3f pos = vec3f{
        t1.get_position().x + (t2.get_position().x - t1.get_position().x) * factor,
        t1.get_position().y + (t2.get_position().y - t1.get_position().y) * factor,
        t1.get_position().z + (t2.get_position().z - t1.get_position().z) * factor
    };
    result.set_position(pos);

    vec3f rot = vec3f{
        t1.get_rotation().x + (t2.get_rotation().x - t1.get_rotation().x) * factor,
        t1.get_rotation().y + (t2.get_rotation().y - t1.get_rotation().y) * factor,
        t1.get_rotation().z + (t2.get_rotation().z - t1.get_rotation().z) * factor
    };
    result.set_rotation(rot);

    vec3f scale = vec3f{
        t1.get_scale().x + (t2.get_scale().x - t1.get_scale().x) * factor,
        t1.get_scale().y + (t2.get_scale().y - t1.get_scale().y) * factor,
        t1.get_scale().z + (t2.get_scale().z - t1.get_scale().z) * factor
    };
    result.set_scale(scale);

    vec3f origin = vec3f{
        t1.get_origin().x + (t2.get_origin().x - t1.get_origin().x) * factor,
        t1.get_origin().y + (t2.get_origin().y - t1.get_origin().y) * factor,
        t1.get_origin().z + (t2.get_origin().z - t1.get_origin().z) * factor
    };
    result.set_origin(origin);

    return result;
}

template <typename... Cs>
animation_system<Cs...>::animation_modifier::animation_modifier(
    animation_system* system,
    entity ent,
    animation_component* component
)
    : system_(system), entity_(ent), component_(component) {}

template <typename... Cs>
auto animation_system<Cs...>::modify(entity ent) -> animation_modifier {
    auto& comp = registry_->template get<animation_component>(ent);
    return animation_modifier(this, ent, &comp);
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::play() {
    if (component_->state_ != animation_state::playing) {
        component_->state_ = animation_state::playing;
        component_->current_time_ = 0.0f;
        component_->direction_ = 1.0f;

        system_->add_active_entity(entity_);
    }
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::pause() {
    if (component_->state_ == animation_state::playing) {
        component_->state_ = animation_state::paused;
    }
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::stop() {
    component_->state_ = animation_state::stopped;
    component_->current_time_ = 0.0f;

    system_->remove_active_entity(entity_);
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::resume() {
    if (component_->state_ == animation_state::paused) {
        component_->state_ = animation_state::playing;
    }
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::set_clip(std::shared_ptr<animation_clip> clip
) {
    component_->clip_ = std::move(clip);
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::set_clip_by_name(const std::string& name) {
    auto clip = system_->clip_registry_->get(name);
    if (clip) {
        set_clip(clip);
    }
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::set_time(float32 time) {
    component_->current_time_ = time;
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::set_playback_speed(float32 speed) {
    component_->playback_speed_ = speed;
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::set_loop_mode(animation_loop_mode mode) {
    component_->loop_mode_ = mode;
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::blend_to(
    std::shared_ptr<animation_clip> clip,
    float32 blend_duration
) {
    component_->previous_clip_ = component_->clip_;
    component_->previous_time_ = component_->current_time_;
    component_->clip_ = std::move(clip);
    component_->current_time_ = 0.0f;
    component_->blend_time_ = 0.0f;
    component_->blend_duration_ = blend_duration;
}

template <typename... Cs>
void animation_system<Cs...>::animation_modifier::blend_to_by_name(
    const std::string& name,
    float32 blend_duration
) {
    auto clip = system_->clip_registry_->get(name);
    if (clip) {
        blend_to(clip, blend_duration);
    }
}

template <typename... Cs>
auto animation_system<Cs...>::animation_modifier::get_clip() const
    -> std::shared_ptr<animation_clip> {
    return component_->get_clip();
}

template <typename... Cs>
auto animation_system<Cs...>::animation_modifier::get_state() const -> animation_state {
    return component_->get_state();
}

template <typename... Cs>
auto animation_system<Cs...>::animation_modifier::get_current_time() const -> float32 {
    return component_->get_current_time();
}

}  // namespace vw::gfx
