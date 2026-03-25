#pragma once

#ifndef VW_GFX_WORLD_GRID_GEN_COLUMN_H
#define VW_GFX_WORLD_GRID_GEN_COLUMN_H

#include <unordered_map>

#include "vw/core.h"
#include "vw/gfx/world_grid/terrain_generator.h"

namespace vw::gfx {

enum class column_phase : uint8 {
    empty,
    terrain,
    complete
};

class gen_column {
public:
    explicit gen_column(int32 cx, int32 cz);

    [[nodiscard]] auto get_coord() const -> vec2i;
    [[nodiscard]] auto get_phase() const -> column_phase;

    [[nodiscard]] auto has_chunk_data(int32 y) const -> bool;
    [[nodiscard]] auto get_chunk_data(int32 y) -> chunk_data*;
    [[nodiscard]] auto get_chunk_data(int32 y) const -> const chunk_data*;

    auto create_chunk(int32 y, chunk_data data) -> chunk_data&;

    [[nodiscard]] auto get_all_chunk_data() -> std::unordered_map<int32, chunk_data>&;

    void set_phase(column_phase phase);

private:
    vec2i coord_;
    column_phase phase_ = column_phase::empty;
    std::unordered_map<int32, chunk_data> chunks_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/gen_column.inl.h"

#endif  // VW_GFX_WORLD_GRID_GEN_COLUMN_H
