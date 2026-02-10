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
    to_remove_.clear();

    for (entity ent : active_entities_) {
        if (!registry_->template has<animation_component>(ent)) {
            to_remove_.push_back(ent);
            continue;
        }

        auto& anim_comp = registry_->template get<animation_component>(ent);

        process_animation(ent, anim_comp, delta_time);

        if (anim_comp.state_ == animation_state::stopped) {
            to_remove_.push_back(ent);
        }
    }

    for (entity ent : to_remove_) {
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

    while (!to_visit_.empty()) {
        to_visit_.pop();
    }
    to_visit_.push(root_ent);

    while (!to_visit_.empty()) {
        entity current = to_visit_.front();
        to_visit_.pop();

        if (registry_->template has<animation_target_component>(current)) {
            const auto& target_comp = registry_->template get<animation_target_component>(current);
            target_map[target_comp.get_name()] = current;
        }

        if (registry_->template has<hierarchy_component>(current)) {
            const auto& hierarchy = registry_->template get<hierarchy_component>(current);
            for (entity child : hierarchy.get_children()) {
                to_visit_.push(child);
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
    if (!anim_comp.clip_ || anim_comp.state_ != animation_state::playing) {
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
            if (anim_comp.direction_ > 0.0f && anim_comp.current_time_ >= duration) {
                anim_comp.current_time_ = 0.0f;
            } else if (anim_comp.direction_ < 0.0f && anim_comp.current_time_ < 0.0f) {
                anim_comp.current_time_ = duration;
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
        auto current_result = track.get_transform(time);
        if (!current_result) {
            return transform{};
        }
        transform current = *current_result;

        if (anim_comp.previous_clip_ && anim_comp.blend_duration_ > 0.0f) {
            auto* prev_track = anim_comp.previous_clip_->get_track(track.get_target_name());
            if (prev_track) {
                auto previous_result = prev_track->get_transform(anim_comp.previous_time_);
                if (previous_result) {
                    transform previous = *previous_result;
                    float32 blend_factor = anim_comp.blend_time_ / anim_comp.blend_duration_;
                    current = blend_transforms(previous, current, blend_factor);
                }
            }
        }

        return current;
    };

    for (const auto& track : anim_comp.clip_->get_tracks()) {
        auto it = target_map->find(track.get_target_name());
        if (it == target_map->end()) {
            continue;
        }

        entity target_ent = it->second;

        if (!registry_->template has<transform_component>(target_ent)) {
            continue;
        }

        float32 frame_time = track.get_frame_time();
        uint32 current_frame = static_cast<uint32>(anim_comp.current_time_ / frame_time);

        auto last_frame_it = last_applied_frame_.find(target_ent);
        if (last_frame_it != last_applied_frame_.end() && last_frame_it->second == current_frame) {
            continue;
        }

        last_applied_frame_[target_ent] = current_frame;

        auto transform_result = track.get_transform(anim_comp.current_time_);
        auto matrix_result = track.get_matrix(anim_comp.current_time_);

        if (!transform_result || !matrix_result) {
            continue;
        }

        transform t = *transform_result;
        mat4f m = *matrix_result;

        if (anim_comp.previous_clip_ && anim_comp.blend_duration_ > 0.0f) {
            auto* prev_track = anim_comp.previous_clip_->get_track(track.get_target_name());
            if (prev_track) {
                auto previous_result = prev_track->get_transform(anim_comp.previous_time_);
                if (previous_result) {
                    transform previous = *previous_result;
                    float32 blend_factor = anim_comp.blend_time_ / anim_comp.blend_duration_;
                    t = blend_transforms(previous, t, blend_factor);
                    m = t.calc_matrix();
                }
            }
        }

        auto modifier = transform_system_->modify(target_ent);
        modifier.set_transform_with_matrix(t, m);
    }
}

template <typename... Cs>
auto animation_system<Cs...>::blend_transforms(
    const transform& t1,
    const transform& t2,
    float32 factor
) const -> transform {
    factor = math::clamp(factor, 0.0f, 1.0f);

    transform result;
    result.set_position(math::lerp(t1.get_position(), t2.get_position(), factor));
    result.set_rotation(math::lerp(t1.get_rotation(), t2.get_rotation(), factor));
    result.set_scale(math::lerp(t1.get_scale(), t2.get_scale(), factor));
    result.set_origin(math::lerp(t1.get_origin(), t2.get_origin(), factor));

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
void animation_system<Cs...>::animation_modifier::set_clip_by_name(std::string_view name) {
    auto clip = system_->clip_registry_->get(std::string(name));
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
    std::string_view name,
    float32 blend_duration
) {
    auto clip = system_->clip_registry_->get(std::string(name));
    if (clip) {
        blend_to(clip, blend_duration);
    }
}

template <typename... Cs>
animation_system<Cs...>::target_modifier::target_modifier(
    entity ent,
    animation_target_component* component
)
    : entity_(ent), component_(component) {}

template <typename... Cs>
auto animation_system<Cs...>::modify_target(entity ent) -> target_modifier {
    auto& comp = registry_->template get<animation_target_component>(ent);
    return target_modifier(ent, &comp);
}

template <typename... Cs>
void animation_system<Cs...>::target_modifier::set_target_name(std::string name) {
    component_->target_name_ = std::move(name);
}

}  // namespace vw::gfx
