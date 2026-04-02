#pragma once

#include <vector>

namespace vw::gfx {

template <typename WC>
animation_system<WC>::animation_system(
    context_type& context,
    transform_system_type& transform_sys,
    animation_clip_registry& clip_registry
)
    : context_(&context), transform_system_(&transform_sys), clip_registry_(&clip_registry) {}

template <typename WC>
auto animation_system<WC>::get_target_fps() const -> float32 {
    return 1.0f / target_frame_time_;
}

template <typename WC>
void animation_system<WC>::set_target_fps(
    float32 fps
) {
    target_frame_time_ = 1.0f / fps;
}


template <typename WC>
void animation_system<WC>::update(
    float32 delta_time
) {
    accumulated_delta_time_ += delta_time;

    if (accumulated_delta_time_ < target_frame_time_) {
        return;
    }

    float32 effective_delta = target_frame_time_;
    accumulated_delta_time_ -= target_frame_time_;
    if (accumulated_delta_time_ > target_frame_time_) {
        accumulated_delta_time_ = target_frame_time_;
    }

    to_remove_.clear();

    for (entity ent : active_entities_) {
        if (!context_->registry().template has<animation_player_component>(ent)) {
            to_remove_.push_back(ent);
            continue;
        }

        auto& anim_comp = context_->registry().template get<animation_player_component>(ent);

        process_animation(ent, anim_comp, effective_delta);

        if (!anim_comp.is_any_playing()) {
            to_remove_.push_back(ent);
        }
    }

    for (entity ent : to_remove_) {
        remove_active_entity(ent);
    }
}

template <typename WC>
void animation_system<WC>::add_active_entity(
    entity root_ent
) {
    auto [it, inserted] = active_entities_.insert(root_ent);

    if (inserted) {
        build_and_cache_target_map(root_ent);
    }
}

template <typename WC>
void animation_system<WC>::remove_active_entity(
    entity root_ent
) {
    active_entities_.erase(root_ent);
    target_maps_.erase(root_ent);
}

template <typename WC>
void animation_system<WC>::build_and_cache_target_map(
    entity root_ent
) {
    std::unordered_map<std::string, entity> target_map;

    to_visit_.clear();
    to_visit_.push_back(root_ent);

    while (!to_visit_.empty()) {
        entity current = to_visit_.front();
        to_visit_.pop_front();

        if (context_->registry().template has<animation_target_component>(current)) {
            const auto& target_comp = context_->registry().template get<animation_target_component>(current);
            target_map[target_comp.get_name()] = current;
        }

        if (context_->registry().template has<hierarchy_component>(current)) {
            const auto& hierarchy = context_->registry().template get<hierarchy_component>(current);
            for (entity child : hierarchy.get_children()) {
                to_visit_.push_back(child);
            }
        }
    }

    target_maps_[root_ent] = std::move(target_map);
}

template <typename WC>
auto animation_system<WC>::get_cached_target_map(
    entity root_ent
) const -> const std::unordered_map<std::string, entity>* {
    auto it = target_maps_.find(root_ent);
    if (it != target_maps_.end()) {
        return &it->second;
    }
    return nullptr;
}

template <typename WC>
void animation_system<WC>::update_layer_time(
    animation_layer& layer, float32 delta_time
) {
    layer.time += delta_time * layer.playback_speed * layer.direction;

    float32 duration = layer.clip->get_duration();

    if (layer.loop_mode == animation_loop_mode::once) {
        if (layer.time >= duration) {
            layer.time  = duration;
            layer.state = animation_state::stopped;
        } else if (layer.time < 0.0f) {
            layer.time  = 0.0f;
            layer.state = animation_state::stopped;
        }
    } else if (layer.loop_mode == animation_loop_mode::loop) {
        if (duration > 0.0f) {
            if (layer.direction > 0.0f && layer.time >= duration) {
                layer.time = 0.0f;
            } else if (layer.direction < 0.0f && layer.time < 0.0f) {
                layer.time = duration;
            }
        }
    } else if (layer.loop_mode == animation_loop_mode::ping_pong) {
        if (layer.time >= duration) {
            layer.direction = -1.0f;
            layer.time      = duration;
        } else if (layer.time <= 0.0f) {
            layer.direction = 1.0f;
            layer.time      = 0.0f;
        }
    }
}

template <typename WC>
void animation_system<WC>::process_layer(
    animation_layer& layer, float32 delta_time, bool is_base
) {
    if (!layer.clip) {
        return;
    }

    if (layer.blend_transition.duration > 0.0f &&
        layer.blend_elapsed < layer.blend_transition.duration) {
        layer.blend_elapsed += delta_time;

        if (layer.blend_prev_clip) {
            layer.blend_prev_time +=
                delta_time * layer.blend_prev_playback_speed * layer.blend_prev_direction;
            float32 prev_duration = layer.blend_prev_clip->get_duration();
            if (layer.blend_prev_time > prev_duration) {
                layer.blend_prev_time = prev_duration;
            } else if (layer.blend_prev_time < 0.0f) {
                layer.blend_prev_time = 0.0f;
            }
        }

        if (layer.blend_elapsed >= layer.blend_transition.duration) {
            layer.blend_prev_clip  = nullptr;
            layer.blend_elapsed    = 0.0f;
            layer.blend_transition = {};
            layer.blend_snapshot.clear();
        }
    }

    if (layer.fade_is_out) {
        layer.fade_elapsed += delta_time;
        if (layer.fade_out.duration > 0.0f && layer.fade_elapsed < layer.fade_out.duration) {
            float32 t = layer.fade_elapsed / layer.fade_out.duration;
            t         = math::apply_easing_bezier(
                t, layer.fade_out.interp, layer.fade_out.tangent_in, layer.fade_out.tangent_out
            );
            layer.fade_influence = 1.0f - t;
        } else {
            layer.fade_influence = 0.0f;
            layer.fade_is_out    = false;
            layer.state          = animation_state::stopped;
        }
        if (layer.state == animation_state::playing) {
            update_layer_time(layer, delta_time);
        }
        return;
    }

    if (layer.state != animation_state::playing) {
        return;
    }

    if (!is_base && layer.fade_influence < 1.0f) {
        if (layer.fade_in.duration > 0.0f) {
            layer.fade_elapsed += delta_time;
            if (layer.fade_elapsed < layer.fade_in.duration) {
                float32 t = layer.fade_elapsed / layer.fade_in.duration;
                t         = math::apply_easing_bezier(
                    t, layer.fade_in.interp, layer.fade_in.tangent_in, layer.fade_in.tangent_out
                );
                layer.fade_influence = t;
            } else {
                layer.fade_influence = 1.0f;
            }
        } else {
            layer.fade_influence = 1.0f;
        }
    }

    update_layer_time(layer, delta_time);

    if (!is_base && layer.state == animation_state::stopped) {
        if (layer.fade_out.duration > 0.0f) {
            layer.fade_is_out  = true;
            layer.fade_elapsed = 0.0f;
        } else {
            layer.fade_influence = 0.0f;
        }
    }
}

template <typename WC>
auto animation_system<WC>::compute_layer_transform(
    const animation_layer& layer, const std::string& target_name
) const -> std::optional<transform> {
    if (!layer.clip) {
        return std::nullopt;
    }

    const auto* track = layer.clip->get_track(target_name);
    if (!track) {
        return std::nullopt;
    }

    auto transform_result = track->get_transform(layer.time);
    if (!transform_result) {
        return std::nullopt;
    }

    transform t = *transform_result;

    bool is_blending = layer.blend_transition.duration > 0.0f &&
        (layer.blend_prev_clip || !layer.blend_snapshot.empty());
    if (is_blending) {
        float32 blend_factor =
            math::clamp(layer.blend_elapsed / layer.blend_transition.duration, 0.0f, 1.0f);
        blend_factor = math::apply_easing_bezier(
            blend_factor,
            layer.blend_transition.interp,
            layer.blend_transition.tangent_in,
            layer.blend_transition.tangent_out
        );

        auto snapshot_it = layer.blend_snapshot.find(target_name);
        if (snapshot_it != layer.blend_snapshot.end()) {
            t = math::lerp(snapshot_it->second, t, blend_factor);
        } else if (layer.blend_prev_clip) {
            auto* prev_track = layer.blend_prev_clip->get_track(target_name);
            if (prev_track) {
                auto prev_result = prev_track->get_transform(layer.blend_prev_time);
                if (prev_result) {
                    t = math::lerp(*prev_result, t, blend_factor);
                }
            }
        }
    }

    return t;
}

template <typename WC>
void animation_system<WC>::process_animation(
    entity ent, animation_player_component& anim_comp, float32 delta_time
) {
    for (size_t i = 0; i < anim_comp.layers_.size(); ++i) {
        process_layer(anim_comp.layers_[i], delta_time, i == 0);
    }

    apply_animation(ent, anim_comp);
}

template <typename WC>
void animation_system<WC>::apply_animation(
    entity root_ent, const animation_player_component& anim_comp
) {
    const auto* target_map = get_cached_target_map(root_ent);
    if (!target_map) {
        return;
    }

    std::unordered_map<std::string, transform> final_transforms;

    if (!anim_comp.layers_.empty()) {
        const auto& base = anim_comp.layers_[0];
        if (base.clip) {
            for (const auto& track : base.clip->get_tracks()) {
                const auto& name = track.get_target_name();
                auto t           = compute_layer_transform(base, name);
                if (t) {
                    final_transforms[name] = *t;
                }
            }

            if (base.blend_transition.duration > 0.0f && base.blend_prev_clip) {
                float32 blend_factor =
                    math::clamp(base.blend_elapsed / base.blend_transition.duration, 0.0f, 1.0f);
                blend_factor = math::apply_easing_bezier(
                    blend_factor,
                    base.blend_transition.interp,
                    base.blend_transition.tangent_in,
                    base.blend_transition.tangent_out
                );

                for (const auto& prev_track : base.blend_prev_clip->get_tracks()) {
                    const auto& name = prev_track.get_target_name();
                    if (final_transforms.contains(name)) {
                        continue;
                    }

                    auto snapshot_it = base.blend_snapshot.find(name);
                    if (snapshot_it != base.blend_snapshot.end()) {
                        transform identity;
                        final_transforms[name] =
                            math::lerp(snapshot_it->second, identity, blend_factor);
                    } else {
                        auto prev_result = prev_track.get_transform(base.blend_prev_time);
                        if (prev_result) {
                            transform identity;
                            final_transforms[name] =
                                math::lerp(*prev_result, identity, blend_factor);
                        }
                    }
                }
            }
        }
    }

    for (size_t i = 1; i < anim_comp.layers_.size(); ++i) {
        const auto& layer = anim_comp.layers_[i];
        if (!layer.clip || layer.fade_influence <= 0.0f) {
            continue;
        }

        for (const auto& target_name : layer.mask) {
            auto t = compute_layer_transform(layer, target_name);
            if (!t) {
                continue;
            }

            auto base_it = final_transforms.find(target_name);
            if (base_it != final_transforms.end()) {
                base_it->second = math::lerp(base_it->second, *t, layer.fade_influence);
            } else {
                transform identity;
                final_transforms[target_name] = math::lerp(identity, *t, layer.fade_influence);
            }
        }
    }

    for (const auto& [name, t] : final_transforms) {
        auto it = target_map->find(name);
        if (it == target_map->end()) {
            continue;
        }

        entity target_ent = it->second;
        if (!context_->registry().template has<transform_component>(target_ent)) {
            continue;
        }

        auto modifier = transform_system_->modify(target_ent);
        modifier.set_transform_with_matrix(t, t.calc_matrix());
    }
}

template <typename WC>
animation_system<WC>::player_modifier::player_modifier(
    animation_system* system, entity ent, animation_player_component* component
)
    : system_(system), entity_(ent), component_(component) {}

template <typename WC>
auto animation_system<WC>::modify_player(
    entity ent
) -> player_modifier {
    auto& comp = context_->registry().template get<animation_player_component>(ent);
    return player_modifier(this, ent, &comp);
}

template <typename WC>
void animation_system<WC>::player_modifier::add_layer(
    size_t index
) const {
    if (index >= component_->layers_.size()) {
        component_->layers_.resize(index + 1);
    }
}


template <typename WC>
auto animation_system<WC>::player_modifier::layer(
    size_t index
) -> layer_modifier {
    add_layer(index);
    return layer_modifier(system_, entity_, &component_->layers_[index]);
}

template <typename WC>
void animation_system<WC>::player_modifier::apply_pose() const {
    if (!system_->get_cached_target_map(entity_)) {
        system_->build_and_cache_target_map(entity_);
    }
    system_->apply_animation(entity_, *component_);
}

template <typename WC>
void animation_system<WC>::player_modifier::rebuild_target_map() const {
    system_->build_and_cache_target_map(entity_);
}

template <typename WC>
animation_system<WC>::layer_modifier::layer_modifier(
    animation_system* system, entity ent, animation_layer* layer
)
    : system_(system), entity_(ent), layer_(layer) {}

template <typename WC>
void animation_system<WC>::layer_modifier::play() const {
    if (layer_->state != animation_state::playing) {
        layer_->state          = animation_state::playing;
        layer_->time           = 0.0f;
        layer_->direction      = 1.0f;
        layer_->fade_influence = 1.0f;
        layer_->fade_is_out    = false;
        layer_->fade_elapsed   = 0.0f;

        system_->add_active_entity(entity_);
    }
}

template <typename WC>
void animation_system<WC>::layer_modifier::play(
    const transition& fade_in
) const {
    layer_->fade_in        = fade_in;
    layer_->fade_influence = 0.0f;
    layer_->fade_is_out    = false;
    layer_->fade_elapsed   = 0.0f;

    if (layer_->state != animation_state::playing) {
        layer_->state     = animation_state::playing;
        layer_->time      = 0.0f;
        layer_->direction = 1.0f;

        system_->add_active_entity(entity_);
    }
}

template <typename WC>
void animation_system<WC>::layer_modifier::pause() const {
    if (layer_->state == animation_state::playing) {
        layer_->state = animation_state::paused;
    }
}

template <typename WC>
void animation_system<WC>::layer_modifier::stop() const {
    layer_->state          = animation_state::stopped;
    layer_->time           = 0.0f;
    layer_->fade_influence = 0.0f;
    layer_->fade_is_out    = false;
}

template <typename WC>
void animation_system<WC>::layer_modifier::clear() const {
    *layer_ = animation_layer{};
}

template <typename WC>
void animation_system<WC>::layer_modifier::stop(
    const transition& fade_out
) const {
    layer_->fade_out     = fade_out;
    layer_->fade_is_out  = true;
    layer_->fade_elapsed = 0.0f;
}

template <typename WC>
void animation_system<WC>::layer_modifier::resume() const {
    if (layer_->state == animation_state::paused) {
        layer_->state = animation_state::playing;
        system_->add_active_entity(entity_);
    }
}

template <typename WC>
void animation_system<WC>::layer_modifier::set_time(
    float32 time
) const {
    layer_->time = time;
}

template <typename WC>
void animation_system<WC>::layer_modifier::set_playback_speed(
    float32 speed
) const {
    layer_->playback_speed = speed;
}

template <typename WC>
void animation_system<WC>::layer_modifier::set_loop_mode(
    animation_loop_mode mode
) const {
    layer_->loop_mode = mode;
}

template <typename WC>
void animation_system<WC>::layer_modifier::set_fade_in(
    const transition& t
) const {
    layer_->fade_in = t;
}

template <typename WC>
void animation_system<WC>::layer_modifier::set_fade_out(
    const transition& t
) const {
    layer_->fade_out = t;
}

template <typename WC>
void animation_system<WC>::layer_modifier::blend_to(
    std::shared_ptr<animation_clip> clip, std::optional<transition> t
) const {
    transition trans = t.value_or(layer_->blend_transition);

    if (trans.duration > 0.0f && layer_->clip) {
        if (layer_->blend_transition.duration > 0.0f && layer_->clip) {
            float32 bf =
                math::clamp(layer_->blend_elapsed / layer_->blend_transition.duration, 0.0f, 1.0f);
            bf = math::apply_easing_bezier(
                bf,
                layer_->blend_transition.interp,
                layer_->blend_transition.tangent_in,
                layer_->blend_transition.tangent_out
            );

            auto old_snapshot = std::move(layer_->blend_snapshot);
            layer_->blend_snapshot.clear();

            for (const auto& track : layer_->clip->get_tracks()) {
                const auto& name = track.get_target_name();
                auto cur_result  = track.get_transform(layer_->time);
                if (!cur_result) {
                    continue;
                }

                transform blended = *cur_result;

                auto snapshot_it = old_snapshot.find(name);
                if (snapshot_it != old_snapshot.end()) {
                    blended = math::lerp(snapshot_it->second, blended, bf);
                } else if (layer_->blend_prev_clip) {
                    auto* prev_track = layer_->blend_prev_clip->get_track(name);
                    if (prev_track) {
                        auto prev_result = prev_track->get_transform(layer_->blend_prev_time);
                        if (prev_result) {
                            blended = math::lerp(*prev_result, blended, bf);
                        }
                    }
                }

                layer_->blend_snapshot[name] = blended;
            }
        } else {
            layer_->blend_snapshot.clear();
        }

        layer_->blend_prev_clip           = layer_->clip;
        layer_->blend_prev_time           = layer_->time;
        layer_->blend_prev_playback_speed = layer_->playback_speed;
        layer_->blend_prev_direction      = layer_->direction;
    } else {
        layer_->blend_prev_clip = nullptr;
        layer_->blend_snapshot.clear();
    }

    layer_->clip             = std::move(clip);
    layer_->time             = 0.0f;
    layer_->blend_elapsed    = 0.0f;
    layer_->blend_transition = trans;

    if (layer_->clip) {
        layer_->mask = layer_->clip->get_target_names();
    } else {
        layer_->mask.clear();
    }

    if (layer_->state != animation_state::playing) {
        layer_->state          = animation_state::playing;
        layer_->direction      = 1.0f;
        layer_->fade_is_out    = false;
        layer_->fade_influence = 1.0f;
        layer_->fade_elapsed   = 0.0f;
    }
    system_->add_active_entity(entity_);
}

template <typename WC>
void animation_system<WC>::layer_modifier::blend_to_by_name(
    std::string_view name, std::optional<transition> t
) {
    auto clip = system_->clip_registry_->get(name);
    if (clip) {
        blend_to(std::move(clip), t);
    }
}

template <typename WC>
animation_system<WC>::target_modifier::target_modifier(
    entity ent, animation_target_component* component
)
    : entity_(ent), component_(component) {}

template <typename WC>
auto animation_system<WC>::modify_target(
    entity ent
) -> target_modifier {
    auto& comp = context_->registry().template get<animation_target_component>(ent);
    return target_modifier(ent, &comp);
}

template <typename WC>
void animation_system<WC>::target_modifier::set_target_name(
    std::string name
) const {
    component_->target_name_ = std::move(name);
}

}  // namespace vw::gfx
