#pragma once

#ifndef VW_GFX_ANIMATION_H
#define VW_GFX_ANIMATION_H

// Агрегирующий заголовок для системы анимаций
// Включает все необходимые файлы для работы с анимациями

// Базовые типы и перечисления
#include "vw/asset/animation/animation_types.h"

// Структуры данных анимации
#include "vw/asset/animation/keyframe.h"
#include "vw/asset/animation/animation_channel.h"
#include "vw/asset/animation/animation_track.h"
#include "vw/asset/animation/animation_clip.h"

// Реестр анимационных клипов
#include "vw/asset/animation/animation_clip_registry.h"

// ECS компоненты
#include "vw/ecs/components/animation_player_component.h"
#include "vw/ecs/components/animation_target_component.h"

// Система анимаций
#include "vw/ecs/systems/animation_system.h"

#endif  // VW_GFX_ANIMATION_H
