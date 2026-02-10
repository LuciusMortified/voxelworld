#pragma once

#ifndef VW_GFX_ANIMATION_H
#define VW_GFX_ANIMATION_H

// Агрегирующий заголовок для системы анимаций
// Включает все необходимые файлы для работы с анимациями

// Базовые типы и перечисления
#include "vw/gfx/animation/animation_types.h"

// Структуры данных анимации
#include "vw/gfx/animation/keyframe.h"
#include "vw/gfx/animation/animation_channel.h"
#include "vw/gfx/animation/animation_track.h"
#include "vw/gfx/animation/animation_clip.h"

// Реестр анимационных клипов
#include "vw/gfx/animation/animation_clip_registry.h"

// ECS компоненты
#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_target_component.h"

// Система анимаций
#include "vw/gfx/world/systems/animation_system.h"

#endif  // VW_GFX_ANIMATION_H
