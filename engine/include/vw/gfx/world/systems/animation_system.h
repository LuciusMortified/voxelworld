#pragma once

#ifndef VW_GFX_ANIMATION_SYSTEM_H
#define VW_GFX_ANIMATION_SYSTEM_H

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/core/transform.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/world/components/animation_component.h"
#include "vw/gfx/world/components/animation_target_component.h"
#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/registry.h"

namespace vw::gfx {

template <typename WC>
class world;

template <typename... Cs>
class transform_system;

// Система анимаций - управляет воспроизведением анимаций
// Оптимизирована для работы только с активными анимациями
template <typename... Cs>
class animation_system final {
public:
    using world_type = world<std::tuple<Cs...>>;
    using registry_type = registry<Cs...>;
    using transform_system_type = transform_system<Cs...>;

    explicit animation_system(
        world_type& world,
        registry_type& registry,
        transform_system_type& transform_sys,
        animation_clip_registry& clip_registry
    );

    // ========== Главный метод обновления ==========

    // Обновить все активные анимации (вызывается каждый кадр)
    void update(float32 delta_time);

    // ========== Animation Modifier ==========

    // Модификатор для безопасного управления animation_component
    class animation_modifier {
    public:
        // Управление воспроизведением
        void play();    // Начать воспроизведение (добавляет в active_entities_)
        void pause();   // Поставить на паузу
        void stop();    // Остановить (удаляет из active_entities_)
        void resume();  // Продолжить с паузы

        // Установка клипа
        void set_clip(std::shared_ptr<animation_clip> clip);
        void set_clip_by_name(const std::string& name);

        // Управление временем
        void set_time(float32 time);
        void set_playback_speed(float32 speed);

        // Режим зацикливания
        void set_loop_mode(animation_loop_mode mode);

        // Блендинг между анимациями
        void blend_to(std::shared_ptr<animation_clip> clip, float32 blend_duration);
        void blend_to_by_name(const std::string& name, float32 blend_duration);

        // Getters
        [[nodiscard]] auto get_clip() const -> std::shared_ptr<animation_clip>;
        [[nodiscard]] auto get_state() const -> animation_state;
        [[nodiscard]] auto get_current_time() const -> float32;

    private:
        friend class animation_system;

        animation_modifier(animation_system* system, entity ent, animation_component* component);

        animation_system* system_;
        entity entity_;
        animation_component* component_;
    };

    // Получить modifier для entity
    auto modify(entity ent) -> animation_modifier;

private:
    // ========== ОПТИМИЗАЦИИ ==========

    // Активные корневые entity с анимациями
    std::unordered_set<entity> active_entities_;

    // Кэш маппингов: root_entity -> (target_name -> target_entity)
    std::unordered_map<entity, std::unordered_map<std::string, entity>> target_maps_;

    // ========== Внутренние методы ==========

    // Добавить entity в активные (вызывается из play())
    void add_active_entity(entity root_ent);

    // Удалить entity из активных (вызывается из stop() или при завершении)
    void remove_active_entity(entity root_ent);

    // Построить и закэшировать target_map для entity
    void build_and_cache_target_map(entity root_ent);

    // Получить закэшированный target_map
    [[nodiscard]] auto get_cached_target_map(entity root_ent) const
        -> const std::unordered_map<std::string, entity>*;

    // Обработка одной анимации
    void process_animation(entity ent, animation_component& anim_comp, float32 delta_time);

    // Применение анимации к transform (использует кэш!)
    void apply_animation_to_transform(entity root_ent, const animation_component& anim_comp);

    // Блендинг трансформаций
    [[nodiscard]] auto blend_transforms(
        const transform& t1,
        const transform& t2,
        float32 factor
    ) const -> transform;

    // ========== Зависимости ==========

    world_type* world_;
    registry_type* registry_;
    transform_system_type* transform_system_;
    animation_clip_registry* clip_registry_;
};

// Template specialization для tuple
template <typename... Cs>
struct animation_system_from_tuple;

template <typename... Cs>
struct animation_system_from_tuple<std::tuple<Cs...>> {
    using type = animation_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/animation_system.inl.h"

#endif  // VW_GFX_ANIMATION_SYSTEM_H
