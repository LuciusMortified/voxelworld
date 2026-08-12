export module vw.world:anim;

import std;

import vw.core;

export namespace vw::asset {

enum class animation_state : uint8 { stopped, playing, paused };

enum class animation_loop_mode : uint8 { once, loop, ping_pong };

enum class animation_property : uint8 { position, rotation, scale, origin };

struct transition {
    float32 duration               = 0.0F;
    math::interpolation_type interp = math::interpolation_type::linear;
    float32 tangent_in             = 0.0F;
    float32 tangent_out            = 1.0F;
};

template <animation_property Prop>
struct animation_property_traits;

template <>
struct animation_property_traits<animation_property::position> {
    using type = vec3f;
};

template <>
struct animation_property_traits<animation_property::rotation> {
    using type = quat;
};

template <>
struct animation_property_traits<animation_property::scale> {
    using type = vec3f;
};

template <>
struct animation_property_traits<animation_property::origin> {
    using type = vec3f;
};

inline constexpr uint32 invalid_keyframe_id = std::numeric_limits<uint32>::max();

template <typename T>
class animation_channel;

template <typename T>
struct keyframe {
    keyframe() = default;
    keyframe(float32 time, T value) : time(time), value(std::move(value)) {}

    float32 time = 0.0F;
    T value{};
    math::interpolation_type interp = math::interpolation_type::linear;
    float32 tangent_in             = 0.0F;
    float32 tangent_out            = 1.0F;

    [[nodiscard]] auto id() const -> uint32 {
        return id_;
    }

    [[nodiscard]] auto operator<(const keyframe& other) const -> bool {
        return time < other.time;
    }

    [[nodiscard]] auto operator==(const keyframe& other) const -> bool {
        return time == other.time;
    }

private:
    friend class animation_channel<T>;
    uint32 id_ = invalid_keyframe_id;
};

using keyframe_vec3f = keyframe<vec3f>;
using keyframe_quat  = keyframe<quat>;

// One animated property of one target: a sorted keyframe list plus the
// interpolation between neighbours.
template <typename T>
class animation_channel final {
public:
    enum class error_type : uint8 { empty };

    explicit animation_channel(animation_property property) : property_(property) {}

    void add(const keyframe<T>& kf) {
        keyframes_.push_back(kf);
        if (keyframes_.back().id_ == invalid_keyframe_id) {
            keyframes_.back().id_ = next_id_++;
        }
        std::sort(keyframes_.begin(), keyframes_.end());
    }

    void replace(uint32 id, const keyframe<T>& new_kf) {
        const auto it =
            std::ranges::find_if(keyframes_, [id](const auto& kf) { return kf.id_ == id; });
        if (it != keyframes_.end()) {
            const uint32 preserved_id = it->id_;
            *it                       = new_kf;
            it->id_                   = preserved_id;
            std::sort(keyframes_.begin(), keyframes_.end());
        }
    }

    void remove(uint32 id) {
        std::erase_if(keyframes_, [id](const auto& kf) { return kf.id_ == id; });
    }

    void set_keyframes(std::vector<keyframe<T>> keyframes) {
        keyframes_ = std::move(keyframes);
        next_id_   = 0;
        for (auto& kf : keyframes_) {
            kf.id_ = next_id_++;
        }
        std::sort(keyframes_.begin(), keyframes_.end());
    }

    void clear() {
        keyframes_.clear();
    }

    [[nodiscard]] auto evaluate(float32 time) const -> std::expected<T, error_type> {
        if (keyframes_.empty()) {
            return std::unexpected(error_type::empty);
        }

        if (keyframes_.size() == 1 || time <= keyframes_.front().time) {
            return keyframes_.front().value;
        }

        if (time >= keyframes_.back().time) {
            return keyframes_.back().value;
        }

        const auto it = std::ranges::lower_bound(
            keyframes_, time, {}, [](const keyframe<T>& kf) { return kf.time; });

        if (it == keyframes_.end()) {
            return keyframes_.back().value;
        }

        if (it == keyframes_.begin()) {
            return keyframes_.front().value;
        }

        const keyframe<T>& kf0 = *(it - 1);
        const keyframe<T>& kf1 = *it;

        const float32 duration = kf1.time - kf0.time;
        if (duration <= 0.0F) {
            return kf0.value;
        }

        const float32 t = (time - kf0.time) / duration;

        return math::interpolate(
            kf0.value, kf1.value, t, kf0.interp, kf0.tangent_in, kf0.tangent_out);
    }

    [[nodiscard]] auto get_duration() const -> std::expected<float32, error_type> {
        if (keyframes_.empty()) {
            return std::unexpected(error_type::empty);
        }
        return keyframes_.back().time;
    }

    [[nodiscard]] auto get_property() const -> animation_property {
        return property_;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return keyframes_.empty();
    }

    [[nodiscard]] auto keyframe_count() const -> std::size_t {
        return keyframes_.size();
    }

    [[nodiscard]] auto get_keyframes() const -> const std::vector<keyframe<T>>& {
        return keyframes_;
    }

    [[nodiscard]] auto get_keyframes_mut() -> std::vector<keyframe<T>>& {
        return keyframes_;
    }

private:
    animation_property property_;
    std::vector<keyframe<T>> keyframes_;
    uint32 next_id_ = 0;
};

using animation_channel_variant = std::variant<animation_channel<vec3f>, animation_channel<quat>>;

template <animation_property Prop>
using animation_channel_for = animation_channel<typename animation_property_traits<Prop>::type>;

template <animation_property Prop>
auto make_animation_channel() -> animation_channel_for<Prop> {
    return animation_channel_for<Prop>(Prop);
}

// All channels of one target, baked to a fixed frame rate on first query and
// re-baked whenever a channel changes.
class animation_track final {
public:
    enum class error_type : uint8 { empty };

    explicit animation_track(std::string target_name, float32 fps = 60.0F);

    template <animation_property Prop>
    void add(animation_channel_for<Prop> channel) {
        add_impl(animation_channel_variant(std::move(channel)));
    }

    void remove_channel(animation_property prop);

    [[nodiscard]] auto get_transform(float32 time) const -> std::expected<transform, error_type>;
    [[nodiscard]] auto get_duration() const -> float32;
    [[nodiscard]] auto get_frame_time() const -> float32;

    [[nodiscard]] auto get_target_name() const -> const std::string& {
        return target_name_;
    }

    [[nodiscard]] auto get_channel(animation_property prop) const
        -> const animation_channel_variant*;
    [[nodiscard]] auto get_channel_mut(animation_property prop) -> animation_channel_variant*;
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;

    [[nodiscard]] auto get_channels() const -> const std::vector<animation_channel_variant>& {
        return channels_;
    }

    [[nodiscard]] auto get_fps() const -> float32 {
        return compiled_fps_;
    }

    void mark_dirty() {
        is_dirty_ = true;
    }

private:
    void add_impl(animation_channel_variant channel);
    void recompile_if_needed() const;

    std::string target_name_;
    std::vector<animation_channel_variant> channels_;

    mutable bool is_dirty_ = true;
    mutable float32 compiled_fps_;
    mutable float32 frame_time_      = 0.0F;
    mutable float32 cached_duration_ = 0.0F;
    mutable std::vector<transform> compiled_transforms_;
};

class animation_clip final {
public:
    explicit animation_clip(std::string name);

    void add_track(animation_track track);

    [[nodiscard]] auto get_track(std::string_view target_name) const -> const animation_track*;
    [[nodiscard]] auto get_track_mut(std::string_view target_name) -> animation_track*;
    [[nodiscard]] auto has_track(std::string_view target_name) const -> bool;

    [[nodiscard]] auto get_tracks() const -> const std::vector<animation_track>& {
        return tracks_;
    }

    void remove_track(std::string_view target_name);

    [[nodiscard]] auto get_duration() const -> float32;

    [[nodiscard]] auto get_name() const -> const std::string& {
        return name_;
    }

    void set_name(std::string name);

    [[nodiscard]] auto get_target_names() const -> std::unordered_set<std::string>;

private:
    std::string name_;
    std::vector<animation_track> tracks_;
};

struct string_hash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view sv) const noexcept -> std::size_t {
        return std::hash<std::string_view>{}(sv);
    }
};

class animation_clip_registry final {
public:
    using map_type = std::
        unordered_map<std::string, std::shared_ptr<animation_clip>, string_hash, std::equal_to<>>;

    [[nodiscard]] auto create(std::string_view name) -> std::shared_ptr<animation_clip>;
    void add(std::string_view name, std::shared_ptr<animation_clip> clip);
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    void remove(std::string_view name);

    [[nodiscard]] auto all() const -> const map_type& {
        return clips_;
    }

    [[nodiscard]] auto count() const -> std::size_t {
        return clips_.size();
    }

    void clear();

private:
    map_type clips_;
};

// An independent player over one clip with a target mask, blended with the
// clip it replaced and faded in or out as a whole.
struct animation_layer {
    std::shared_ptr<animation_clip> clip;
    animation_state state         = animation_state::stopped;
    float32 time                  = 0.0F;
    float32 playback_speed        = 1.0F;
    animation_loop_mode loop_mode = animation_loop_mode::once;
    float32 direction             = 1.0F;

    std::shared_ptr<animation_clip> blend_prev_clip;
    float32 blend_prev_time           = 0.0F;
    float32 blend_prev_playback_speed = 1.0F;
    float32 blend_prev_direction      = 1.0F;
    float32 blend_elapsed             = 0.0F;
    transition blend_transition;
    std::unordered_map<std::string, transform> blend_snapshot;

    float32 fade_influence = 0.0F;
    float32 fade_elapsed   = 0.0F;
    transition fade_in;
    transition fade_out;
    bool fade_is_out = false;

    std::unordered_set<std::string> mask;

    [[nodiscard]] auto is_active() const -> bool {
        if (state == animation_state::playing || fade_is_out) {
            return true;
        }
        return blend_transition.duration > 0.0F && blend_elapsed < blend_transition.duration;
    }

    [[nodiscard]] auto is_blending() const -> bool {
        return blend_prev_clip && blend_elapsed < blend_transition.duration;
    }

    [[nodiscard]] auto affects_target(const std::string& name) const -> bool {
        return mask.contains(name);
    }
};

class animation_fsm final {
public:
    using condition_fn = std::function<bool()>;

    struct transition_rule {
        std::string target_state;
        condition_fn condition;
        std::string trigger_name;
        transition blend;
        bool wait_until_end   = false;
        bool wait_until_blend = false;
    };

    struct state_node {
        std::string name;
        std::shared_ptr<animation_clip> clip;
        animation_loop_mode loop_mode = animation_loop_mode::loop;
        float32 playback_speed        = 1.0F;
        transition layer_blend_in;
        transition layer_blend_out;
        std::vector<transition_rule> transitions;
    };

    struct transition_result {
        std::string target_state;
        std::shared_ptr<animation_clip> clip;
        animation_loop_mode loop_mode;
        float32 playback_speed;
        transition blend;
        transition layer_blend_in;
        transition layer_blend_out;
    };

    using trigger_set = std::unordered_set<std::string>;

    void add_state(state_node state);
    void set_entry_state(std::string_view name);

    [[nodiscard]] auto get_current_state() const -> const std::string& {
        return current_state_;
    }

    [[nodiscard]] auto get_current_state_node() const -> const state_node*;
    [[nodiscard]] auto evaluate(const animation_layer& layer, trigger_set& triggers)
        -> std::optional<transition_result>;

    void apply_transition(const transition_result& result);

private:
    std::string current_state_;
    std::string entry_state_;
    std::unordered_map<std::string, state_node> states_;
};

}  // namespace vw::asset
