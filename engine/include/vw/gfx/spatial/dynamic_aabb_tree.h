#pragma once

#ifndef VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H
#define VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/spatial/aabb.h"
#include "vw/gfx/spatial/frustum.h"
#include "vw/gfx/spatial/ray.h"
#include "vw/gfx/world/entity.h"

namespace vw::gfx {

class dynamic_aabb_tree {
public:
    explicit dynamic_aabb_tree();
    
    void insert(entity e, const aabb& bounds);
    void remove(entity e);
    void update(entity e, const aabb& new_bounds);
    
    // Методы запросов - возвращают все попавшие сущности
    void query_all(
        const frustum& f,
        std::unordered_set<entity>& result_out
    ) const;
    
    void query_all(
        const ray& r,
        std::unordered_set<entity>& result_out
    ) const;
    
    void query_all(
        const aabb& bounds,
        std::unordered_set<entity>& result_out
    ) const;
    
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto empty() const -> bool;
    
    void clear();

private:
    struct node {
        aabb bounds;
        entity entity_id = invalid_entity;
        uint32 parent     = invalid_node_index;
        uint32 left       = invalid_node_index;
        uint32 right      = invalid_node_index;
        bool is_leaf      = false;
        
    };
    
    static constexpr uint32 invalid_node_index = std::numeric_limits<uint32>::max();
    static constexpr size_t max_stack_size = 256;  // Максимальный размер стека для итеративного обхода
    
    std::vector<node> nodes_;
    std::vector<uint32> free_nodes_;  // Пул свободных узлов для переиспользования
    uint32 root_index_ = invalid_node_index;
    std::unordered_map<entity, uint32> entity_to_node_;  // Маппинг entity -> индекс узла
    
    [[nodiscard]] auto allocate_node() -> uint32;
    void free_node(uint32 index);
    
    [[nodiscard]] auto find_best_sibling(uint32 new_node_index) const -> uint32;
    void insert_leaf(uint32 leaf_index);
    void remove_leaf(uint32 leaf_index);
    void refit_aabb(uint32 index);
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/dynamic_aabb_tree.inl.h"

#endif  // VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_H
