#pragma once

#ifndef VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H
#define VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H

#include <span>
#include <unordered_map>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/world/entity.h"
#include "vw/gfx/world/spatial_layer.h"
#include "vw/spatial/aabb.h"
#include "vw/spatial/frustum.h"
#include "vw/spatial/ray.h"

namespace vw::gfx {


class dynamic_aabb_tree {
public:
    explicit dynamic_aabb_tree();
    
    void insert(entity e, const vw::spatial::aabb& bounds, spatial_layer_mask layer = spatial_layer::all);
    void remove(entity e);
    void update(entity e, const vw::spatial::aabb& new_bounds, spatial_layer_mask layer = spatial_layer::all);
    
    // Методы запросов - возвращают все попавшие сущности
    void query_all(
        const vw::spatial::frustum& f,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const vw::spatial::ray& r,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const vw::spatial::aabb& bounds,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all_any(
        std::span<const vw::spatial::frustum> frustums,
        std::vector<entity>& result_out
    ) const;

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto empty() const -> bool;
    
    void clear();

private:
    struct node {
        vw::spatial::aabb bounds;
        entity entity_id          = invalid_entity;
        spatial_layer_mask layer   = spatial_layer::none;
        uint32 parent             = invalid_node_index;
        uint32 left               = invalid_node_index;
        uint32 right              = invalid_node_index;
        int32 height              = 0;
        bool is_leaf              = false;
    };

    struct sibling_candidate {
        uint32 index;
        float32 inherited_cost;
    };
    
    static constexpr uint32 invalid_node_index = std::numeric_limits<uint32>::max();

    std::vector<node> nodes_;
    std::vector<uint32> free_nodes_;
    uint32 root_index_ = invalid_node_index;
    std::unordered_map<entity, uint32> entity_to_node_;
    mutable std::vector<uint32> query_stack_;
    mutable std::vector<sibling_candidate> sibling_stack_;

    [[nodiscard]] auto allocate_node() -> uint32;
    void free_node(uint32 index);

    [[nodiscard]] auto find_best_sibling(uint32 new_node_index) const -> uint32;
    void insert_leaf(uint32 leaf_index);
    void remove_leaf(uint32 leaf_index);
    void refit(uint32 index);
    void rotate(uint32 index);
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/dynamic_aabb_tree.inl.h"

#endif  // VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H
