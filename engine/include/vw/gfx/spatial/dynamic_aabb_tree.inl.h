#pragma once

#ifndef VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H
#define VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H

#include <algorithm>
#include <array>
#include <limits>

#include "vw/core/math.h"
#include "vw/gfx/spatial/dynamic_aabb_tree.h"

namespace vw::gfx {

inline dynamic_aabb_tree::dynamic_aabb_tree() {
    nodes_.reserve(256);
    free_nodes_.reserve(64);
    entity_to_node_.reserve(128);
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
    
    const node& new_node = nodes_[new_node_index];
    const aabb& new_bounds = new_node.bounds;
    
    uint32 best_sibling = invalid_node_index;
    float best_cost = std::numeric_limits<float>::max();
    
    // Рекурсивно ищем лучший лист для вставки
    // Используем алгоритм, который выбирает путь, минимизирующий увеличение площади AABB
    std::vector<uint32> stack;
    stack.push_back(root_index_);
    
    while (!stack.empty()) {
        uint32 current = stack.back();
        stack.pop_back();
        
        const node& current_node = nodes_[current];
        
        if (current_node.is_leaf) {
            // Вычислить стоимость вставки в этот лист
            aabb union_bounds = aabb::merge(current_node.bounds, new_bounds);
            float cost = union_bounds.area();
            
            if (cost < best_cost) {
                best_cost = cost;
                best_sibling = current;
            }
        } else {
            // Вычислить стоимость вставки в левое и правое поддеревья
            const node& left_node = nodes_[current_node.left];
            const node& right_node = nodes_[current_node.right];
            
            aabb left_union = aabb::merge(left_node.bounds, new_bounds);
            aabb right_union = aabb::merge(right_node.bounds, new_bounds);
            
            float left_cost = left_union.area();
            float right_cost = right_union.area();
            
            // Выбрать более дешевый путь и продолжить поиск там
            // Также добавить другой путь для проверки
            if (left_cost < right_cost) {
                stack.push_back(current_node.left);
                if (right_cost < best_cost) {
                    stack.push_back(current_node.right);
                }
            } else {
                stack.push_back(current_node.right);
                if (left_cost < best_cost) {
                    stack.push_back(current_node.left);
                }
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
    
    // Обновить AABB нового родителя
    new_parent_node.bounds = aabb::merge(nodes_[sibling].bounds, nodes_[leaf_index].bounds);
    
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
    refit_aabb(new_parent);
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
        refit_aabb(grandparent);
    } else {
        // Родитель был корнем, брат становится новым корнем
        root_index_ = sibling;
        nodes_[sibling].parent = invalid_node_index;
    }
    
    // Освободить узел родителя
    free_node(parent);
}

inline void dynamic_aabb_tree::refit_aabb(uint32 index) {
    while (index != invalid_node_index) {
        node& current = nodes_[index];
        if (!current.is_leaf) {
            current.bounds = aabb::merge(
                nodes_[current.left].bounds,
                nodes_[current.right].bounds
            );
        }
        index = current.parent;
    }
}

inline void dynamic_aabb_tree::insert(entity e, const aabb& bounds) {
    if (entity_to_node_.find(e) != entity_to_node_.end()) {
        // Entity уже существует, обновить
        update(e, bounds);
        return;
    }
    
    uint32 new_node = allocate_node();
    node& new_node_data = nodes_[new_node];
    new_node_data.bounds = bounds;
    new_node_data.entity_id = e;
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

inline void dynamic_aabb_tree::update(entity e, const aabb& new_bounds) {
    auto it = entity_to_node_.find(e);
    if (it == entity_to_node_.end()) {
        // Entity не существует, вставить
        insert(e, new_bounds);
        return;
    }
    
    uint32 node_index = it->second;
    node& node_data = nodes_[node_index];
    
    // Проверить, нужно ли обновлять дерево
    if (node_data.bounds.min.x <= new_bounds.min.x && node_data.bounds.min.y <= new_bounds.min.y &&
        node_data.bounds.min.z <= new_bounds.min.z && node_data.bounds.max.x >= new_bounds.max.x &&
        node_data.bounds.max.y >= new_bounds.max.y && node_data.bounds.max.z >= new_bounds.max.z) {
        // Новый AABB полностью внутри старого, просто обновить
        node_data.bounds = new_bounds;
        return;
    }
    
    // Удалить и вставить заново
    remove_leaf(node_index);
    node_data.bounds = new_bounds;
    insert_leaf(node_index);
}

inline void dynamic_aabb_tree::query_all(
    const frustum& f,
    std::unordered_set<entity>& result_out
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }
    
    // Стек для итеративного обхода дерева (без динамических аллокаций)
    std::array<uint32, max_stack_size> stack;
    size_t stack_top = 0;
    
    stack[stack_top++] = root_index_;
    
    while (stack_top > 0) {
        uint32 node_index = stack[--stack_top];
        const node& current = nodes_[node_index];
        
        // Проверить пересечение AABB узла с frustum
        if (!f.intersects(current.bounds)) {
            continue;
        }
        
        if (current.is_leaf) {
            // Это лист, добавить entity в результат
            if (current.entity_id.is_valid()) {
                result_out.insert(current.entity_id);
            }
        } else {
            // Добавить оба поддерева в стек
            if (stack_top + 1 < max_stack_size) {
                stack[stack_top++] = current.left;
                stack[stack_top++] = current.right;
            } else if (stack_top < max_stack_size) {
                // Если осталось место только для одного узла, добавить хотя бы один
                stack[stack_top++] = current.left;
            }
            // Если стек переполнен, пропускаем (крайне редкий случай для нормального дерева)
        }
    }
}

inline void dynamic_aabb_tree::query_all(
    const ray& r,
    std::unordered_set<entity>& result_out
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }
    
    // Стек для итеративного обхода дерева (без динамических аллокаций)
    std::array<uint32, max_stack_size> stack;
    size_t stack_top = 0;
    
    stack[stack_top++] = root_index_;
    
    while (stack_top > 0) {
        uint32 node_index = stack[--stack_top];
        const node& current = nodes_[node_index];
        
        // Проверить пересечение AABB узла с лучом
        if (!current.bounds.intersects(r)) {
            continue;
        }
        
        if (current.is_leaf) {
            // Это лист, добавить entity в результат
            if (current.entity_id.is_valid()) {
                result_out.insert(current.entity_id);
            }
        } else {
            // Добавить оба поддерева в стек
            if (stack_top + 1 < max_stack_size) {
                stack[stack_top++] = current.left;
                stack[stack_top++] = current.right;
            } else if (stack_top < max_stack_size) {
                // Если осталось место только для одного узла, добавить хотя бы один
                stack[stack_top++] = current.left;
            }
            // Если стек переполнен, пропускаем (крайне редкий случай для нормального дерева)
        }
    }
}

inline void dynamic_aabb_tree::query_all(
    const aabb& bounds,
    std::unordered_set<entity>& result_out
) const {
    result_out.clear();
    if (root_index_ == invalid_node_index) {
        return;
    }
    
    // Стек для итеративного обхода дерева (без динамических аллокаций)
    std::array<uint32, max_stack_size> stack;
    size_t stack_top = 0;
    
    stack[stack_top++] = root_index_;
    
    while (stack_top > 0) {
        uint32 node_index = stack[--stack_top];
        const node& current = nodes_[node_index];
        
        // Проверить пересечение AABB узла с запрашиваемым AABB
        if (!current.bounds.intersects(bounds)) {
            continue;
        }
        
        if (current.is_leaf) {
            // Это лист, добавить entity в результат
            if (current.entity_id.is_valid()) {
                result_out.insert(current.entity_id);
            }
        } else {
            // Добавить оба поддерева в стек
            if (stack_top + 1 < max_stack_size) {
                stack[stack_top++] = current.left;
                stack[stack_top++] = current.right;
            } else if (stack_top < max_stack_size) {
                // Если осталось место только для одного узла, добавить хотя бы один
                stack[stack_top++] = current.left;
            }
            // Если стек переполнен, пропускаем (крайне редкий случай для нормального дерева)
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_SPATIAL_DYNAMIC_AABB_TREE_INL_H
