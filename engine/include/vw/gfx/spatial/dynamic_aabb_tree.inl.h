#pragma once

#ifndef VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H
#define VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H

#include <algorithm>
#include <limits>

#include "vw/core/math.h"
#include "vw/gfx/spatial/dynamic_aabb_tree.h"
#include "vw/spatial/aabb.h"

namespace vw::gfx {

inline dynamic_aabb_tree::dynamic_aabb_tree() {
    nodes_.reserve(256);
    free_nodes_.reserve(64);
    entity_to_node_.reserve(128);
    query_stack_.reserve(256);
    sibling_stack_.reserve(256);
}

inline auto dynamic_aabb_tree::allocate_node() -> uint32 {
    // Сначала проверить пул свободных узлов
    if (!free_nodes_.empty()) {
        uint32 index = free_nodes_.back();
        free_nodes_.pop_back();
        
        // Сбросить узел в начальное состояние
        nodes_[index] = node{};
        return index;
    }
    
    // Если пул пуст, выделить новый узел
    if (nodes_.size() >= nodes_.capacity()) {
        nodes_.reserve(nodes_.capacity() * 2);
    }
    nodes_.emplace_back();
    return static_cast<uint32>(nodes_.size() - 1);
}

inline void dynamic_aabb_tree::free_node(uint32 index) {
    if (index >= nodes_.size()) {
        return;
    }
    
    // Сбросить узел в начальное состояние
    nodes_[index] = node{};
    
    // Добавить в пул свободных узлов для переиспользования
    free_nodes_.push_back(index);
}

inline auto dynamic_aabb_tree::size() const -> size_t {
    return entity_to_node_.size();
}

inline auto dynamic_aabb_tree::empty() const -> bool {
    return entity_to_node_.empty();
}

inline void dynamic_aabb_tree::clear() {
    nodes_.clear();
    free_nodes_.clear();
    entity_to_node_.clear();
    root_index_ = invalid_node_index;
}

inline auto dynamic_aabb_tree::find_best_sibling(uint32 new_node_index) const -> uint32 {
    if (root_index_ == invalid_node_index) {
        return invalid_node_index;
    }

    const aabb& new_bounds = nodes_[new_node_index].bounds;

    uint32 best_sibling = invalid_node_index;
    float32 best_cost = std::numeric_limits<float32>::max();

    sibling_stack_.clear();
    sibling_stack_.push_back({root_index_, 0.0f});

    while (!sibling_stack_.empty()) {
        auto [current, inherited] = sibling_stack_.back();
        sibling_stack_.pop_back();

        const node& current_node = nodes_[current];
        float32 direct_cost = aabb::merge(current_node.bounds, new_bounds).area();
        float32 total_cost = direct_cost + inherited;

        if (inherited >= best_cost) {
            continue;
        }

        if (current_node.is_leaf) {
            if (total_cost < best_cost) {
                best_cost = total_cost;
                best_sibling = current;
            }
        } else {
            if (total_cost < best_cost) {
                best_cost = total_cost;
                best_sibling = current;
            }

            float32 delta = direct_cost - current_node.bounds.area();
            float32 child_inherited = inherited + delta;

            if (child_inherited < best_cost) {
                sibling_stack_.push_back({current_node.left, child_inherited});
                sibling_stack_.push_back({current_node.right, child_inherited});
            }
        }
    }

    return best_sibling;
}

inline void dynamic_aabb_tree::insert_leaf(uint32 leaf_index) {
    if (root_index_ == invalid_node_index) {
        root_index_ = leaf_index;
        return;
    }
    
    // Найти лучший лист для вставки
    uint32 sibling = find_best_sibling(leaf_index);
    
    if (sibling == invalid_node_index) {
        // Если не нашли, просто делаем новый узел корнем
        root_index_ = leaf_index;
        return;
    }
    
    // Создать новый внутренний узел
    uint32 old_parent = nodes_[sibling].parent;
    uint32 new_parent = allocate_node();
    
    node& new_parent_node = nodes_[new_parent];
    new_parent_node.parent = old_parent;
    new_parent_node.left = sibling;
    new_parent_node.right = leaf_index;
    new_parent_node.is_leaf = false;
    
    // Обновить родителя старого листа
    nodes_[sibling].parent = new_parent;
    nodes_[leaf_index].parent = new_parent;
    
    new_parent_node.bounds = aabb::merge(nodes_[sibling].bounds, nodes_[leaf_index].bounds);
    new_parent_node.layer = nodes_[sibling].layer | nodes_[leaf_index].layer;
    
    // Обновить ссылку на родителя в старом родителе
    if (old_parent != invalid_node_index) {
        node& old_parent_node = nodes_[old_parent];
        if (old_parent_node.left == sibling) {
            old_parent_node.left = new_parent;
        } else {
            old_parent_node.right = new_parent;
        }
    } else {
        root_index_ = new_parent;
    }
    
    // Пересчитать AABB вверх по дереву
    refit(new_parent);
}

inline void dynamic_aabb_tree::remove_leaf(uint32 leaf_index) {
    if (leaf_index == root_index_) {
        root_index_ = invalid_node_index;
        return;
    }
    
    uint32 parent = nodes_[leaf_index].parent;
    if (parent == invalid_node_index) {
        return;
    }
    
    uint32 grandparent = nodes_[parent].parent;
    uint32 sibling = (nodes_[parent].left == leaf_index) ? nodes_[parent].right : nodes_[parent].left;
    
    if (grandparent != invalid_node_index) {
        // Заменить родителя на брата
        node& grandparent_node = nodes_[grandparent];
        if (grandparent_node.left == parent) {
            grandparent_node.left = sibling;
        } else {
            grandparent_node.right = sibling;
        }
        nodes_[sibling].parent = grandparent;
        
        // Пересчитать AABB вверх
        refit(grandparent);
    } else {
        // Родитель был корнем, брат становится новым корнем
        root_index_ = sibling;
        nodes_[sibling].parent = invalid_node_index;
    }
    
    // Освободить узел родителя
    free_node(parent);
}

inline void dynamic_aabb_tree::refit(uint32 index) {
    while (index != invalid_node_index) {
        node& current = nodes_[index];
        if (!current.is_leaf) {
            const auto& left = nodes_[current.left];
            const auto& right = nodes_[current.right];

            current.bounds = aabb::merge(left.bounds, right.bounds);
            current.layer = left.layer | right.layer;
            current.height = std::max(left.height, right.height) + 1;

            int32 balance = right.height - left.height;
            if (balance > 1 || balance < -1) {
                rotate(index);
            }
        }
        index = nodes_[index].parent;
    }
}

inline void dynamic_aabb_tree::rotate(uint32 a_idx) {
    node& a = nodes_[a_idx];
    int32 balance = nodes_[a.right].height - nodes_[a.left].height;

    uint32 b_idx = (balance > 0) ? a.right : a.left;
    node& b = nodes_[b_idx];

    uint32 gc1 = b.left;
    uint32 gc2 = b.right;
    uint32 promote = (nodes_[gc1].height <= nodes_[gc2].height) ? gc2 : gc1;
    uint32 demote  = (promote == gc1) ? gc2 : gc1;

    uint32 a_parent = a.parent;

    b.parent = a_parent;
    if (a_parent != invalid_node_index) {
        if (nodes_[a_parent].left == a_idx) {
            nodes_[a_parent].left = b_idx;
        } else {
            nodes_[a_parent].right = b_idx;
        }
    } else {
        root_index_ = b_idx;
    }

    if (balance > 0) {
        a.right = demote;
    } else {
        a.left = demote;
    }
    nodes_[demote].parent = a_idx;

    if (b.left == promote) {
        b.left = promote;
        b.right = a_idx;
    } else {
        b.right = promote;
        b.left = a_idx;
    }
    a.parent = b_idx;

    const auto& al = nodes_[a.left];
    const auto& ar = nodes_[a.right];
    a.bounds = aabb::merge(al.bounds, ar.bounds);
    a.layer = al.layer | ar.layer;
    a.height = std::max(al.height, ar.height) + 1;

    const auto& bl = nodes_[b.left];
    const auto& br = nodes_[b.right];
    b.bounds = aabb::merge(bl.bounds, br.bounds);
    b.layer = bl.layer | br.layer;
    b.height = std::max(bl.height, br.height) + 1;
}

inline void dynamic_aabb_tree::insert(entity e, const aabb& bounds, spatial_layer_mask layer) {
    if (entity_to_node_.find(e) != entity_to_node_.end()) {
        update(e, bounds, layer);
        return;
    }

    uint32 new_node = allocate_node();
    node& new_node_data = nodes_[new_node];
    new_node_data.bounds = bounds;
    new_node_data.entity_id = e;
    new_node_data.layer = layer;
    new_node_data.is_leaf = true;
    new_node_data.parent = invalid_node_index;
    new_node_data.left = invalid_node_index;
    new_node_data.right = invalid_node_index;

    insert_leaf(new_node);
    entity_to_node_[e] = new_node;
}

inline void dynamic_aabb_tree::remove(entity e) {
    auto it = entity_to_node_.find(e);
    if (it == entity_to_node_.end()) {
        return;
    }
    
    uint32 node_index = it->second;
    remove_leaf(node_index);
    entity_to_node_.erase(it);
    free_node(node_index);
}

inline void dynamic_aabb_tree::update(entity e, const aabb& new_bounds, spatial_layer_mask layer) {
    auto it = entity_to_node_.find(e);
    if (it == entity_to_node_.end()) {
        insert(e, new_bounds, layer);
        return;
    }

    uint32 node_index = it->second;
    node& node_data = nodes_[node_index];

    bool layer_changed = node_data.layer != layer;
    node_data.layer = layer;

    if (!layer_changed &&
        node_data.bounds.min.x <= new_bounds.min.x && node_data.bounds.min.y <= new_bounds.min.y &&
        node_data.bounds.min.z <= new_bounds.min.z && node_data.bounds.max.x >= new_bounds.max.x &&
        node_data.bounds.max.y >= new_bounds.max.y && node_data.bounds.max.z >= new_bounds.max.z) {
        node_data.bounds = new_bounds;
        return;
    }

    remove_leaf(node_index);
    node_data.bounds = new_bounds;
    insert_leaf(node_index);
}

inline void dynamic_aabb_tree::query_all(
    const frustum& f,
    std::vector<entity>& result_out,
    spatial_layer_mask layer_mask
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }

    query_stack_.clear();
    query_stack_.push_back(root_index_);

    while (!query_stack_.empty()) {
        uint32 node_index = query_stack_.back();
        query_stack_.pop_back();
        const node& current = nodes_[node_index];

        if (!(current.layer & layer_mask) || !f.intersects(current.bounds)) {
            continue;
        }

        if (current.is_leaf) {
            if (current.entity_id.is_valid()) {
                result_out.push_back(current.entity_id);
            }
        } else {
            query_stack_.push_back(current.left);
            query_stack_.push_back(current.right);
        }
    }
}

inline void dynamic_aabb_tree::query_all_any(
    std::span<const frustum> frustums,
    std::vector<entity>& result_out
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }

    query_stack_.clear();
    query_stack_.push_back(root_index_);

    while (!query_stack_.empty()) {
        uint32 node_index = query_stack_.back();
        query_stack_.pop_back();
        const node& current = nodes_[node_index];

        bool any_intersects = false;
        for (const auto& f : frustums) {
            if (f.intersects(current.bounds)) {
                any_intersects = true;
                break;
            }
        }
        if (!any_intersects) {
            continue;
        }

        if (current.is_leaf) {
            if (current.entity_id.is_valid()) {
                result_out.push_back(current.entity_id);
            }
        } else {
            query_stack_.push_back(current.left);
            query_stack_.push_back(current.right);
        }
    }
}

inline void dynamic_aabb_tree::query_all(
    const ray& r,
    std::vector<entity>& result_out,
    spatial_layer_mask layer_mask
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }

    query_stack_.clear();
    query_stack_.push_back(root_index_);

    while (!query_stack_.empty()) {
        uint32 node_index = query_stack_.back();
        query_stack_.pop_back();
        const node& current = nodes_[node_index];

        if (!(current.layer & layer_mask) || !current.bounds.intersects(r)) {
            continue;
        }

        if (current.is_leaf) {
            if (current.entity_id.is_valid()) {
                result_out.push_back(current.entity_id);
            }
        } else {
            query_stack_.push_back(current.left);
            query_stack_.push_back(current.right);
        }
    }
}

inline void dynamic_aabb_tree::query_all(
    const aabb& bounds,
    std::vector<entity>& result_out,
    spatial_layer_mask layer_mask
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }

    query_stack_.clear();
    query_stack_.push_back(root_index_);

    while (!query_stack_.empty()) {
        uint32 node_index = query_stack_.back();
        query_stack_.pop_back();
        const node& current = nodes_[node_index];

        if (((current.layer & layer_mask) == 0) || !current.bounds.intersects(bounds)) {
            continue;
        }

        if (current.is_leaf) {
            if (current.entity_id.is_valid()) {
                result_out.push_back(current.entity_id);
            }
        } else {
            query_stack_.push_back(current.left);
            query_stack_.push_back(current.right);
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H
