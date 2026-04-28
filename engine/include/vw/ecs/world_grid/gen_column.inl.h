#pragma once

#ifndef VW_ECS_WORLD_GRID_GEN_COLUMN_INL_H
#define VW_ECS_WORLD_GRID_GEN_COLUMN_INL_H

#include <algorithm>
#include <limits>

namespace vw::ecs {

inline gen_column::gen_column(int32 cx, int32 cz)
    : coord_{cx, cz} {}

inline auto gen_column::get_coord() const -> vec2i {
    return coord_;
}

inline auto gen_column::get_phase() const -> column_phase {
    return phase_;
}

inline auto gen_column::has_chunk_data(int32 y) const -> bool {
    return chunks_.contains(y);
}

inline auto gen_column::get_chunk_data(int32 y) -> chunk_data* {
    auto it = chunks_.find(y);
    return it != chunks_.end() ? &it->second : nullptr;
}

inline auto gen_column::get_chunk_data(int32 y) const -> const chunk_data* {
    auto it = chunks_.find(y);
    return it != chunks_.end() ? &it->second : nullptr;
}

inline auto gen_column::create_chunk(int32 y, chunk_data data) -> chunk_data& {
    auto [it, inserted] = chunks_.emplace(y, std::move(data));
    return it->second;
}

inline auto gen_column::get_all_chunk_data() -> std::unordered_map<int32, chunk_data>& {
    return chunks_;
}

inline void gen_column::set_phase(column_phase phase) {
    phase_ = phase;
}

}  // namespace vw::ecs

#endif  // VW_ECS_WORLD_GRID_GEN_COLUMN_INL_H
