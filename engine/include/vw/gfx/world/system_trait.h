#pragma once

#ifndef VW_GFX_WORLD_SYSTEM_TRAIT_H
#define VW_GFX_WORLD_SYSTEM_TRAIT_H

#include <tuple>

namespace vw::gfx {

/// Type list for template template parameters (systems).
template <template <typename> class... Ss>
struct system_list {};

/// Metadata for a system: what components it owns, what it depends on, what resources it needs.
/// Specialize for each concrete system template.
template <template <typename> class S>
struct system_trait {
    using components = std::tuple<>;
    using depends_on = system_list<>;
    using resources  = std::tuple<>;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEM_TRAIT_H
