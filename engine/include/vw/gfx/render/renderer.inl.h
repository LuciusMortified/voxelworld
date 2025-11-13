#pragma once

#ifndef VW_GFX_RENDER_RENDERER_INL_H
#define VW_GFX_RENDER_RENDERER_INL_H

#include "vw/gfx/render/renderer.h"

namespace vw::gfx {

inline void renderer::draw_line(const vec3f& a, const vec3f& b, color col) {
    debug_primitives_.add_line(a, b, col);
}

inline void renderer::draw_box(const vw::transform& transform, const vec3f& size, color col) {
    debug_primitives_.add_box(transform, size, col);
}

inline void renderer::draw_box(const vec3f& position, const vec3f& size, color col) {
    debug_primitives_.add_box(position, size, col);
}

inline void renderer::draw_grid(const vw::transform& transform, float cell_size, int cols, int rows, color clr) {
    debug_primitives_.add_grid(transform, cell_size, cols, rows, clr);
}

inline void renderer::draw_grid(const vec3f& position, float cel_size, int cols, int rows, color clr) {
    debug_primitives_.add_grid(position, cel_size, cols, rows, clr);
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_RENDERER_INL_H

