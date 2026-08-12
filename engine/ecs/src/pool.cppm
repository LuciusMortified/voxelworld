export module vw.ecs:pool;

import std;

import vw.core;
import :entity;

export namespace vw::ecs {

// Everything component_pool needs to know about a type whose identity it erased.
struct component_ops {
    uint32 size;
    uint32 align;
    void (*destroy)(void*);
    void (*relocate)(void* dst, void* src);
};

template <typename T>
constexpr auto ops_of() -> component_ops {
    return {
        .size     = static_cast<uint32>(sizeof(T)),
        .align    = static_cast<uint32>(alignof(T)),
        .destroy  = std::is_trivially_destructible_v<T>
             ? nullptr
             : +[](void* p) { std::destroy_at(static_cast<T*>(p)); },
        .relocate = +[](void* dst, void* src) {
            std::construct_at(static_cast<T*>(dst), std::move(*static_cast<T*>(src)));
            std::destroy_at(static_cast<T*>(src));
        },
    };
}

// Sparse set over type-erased storage: components stay in one dense array,
// entity -> dense index goes through the sparse table. Removal swaps with the
// last element, so dense order is unspecified but iteration stays contiguous.
class component_pool final {
public:
    explicit component_pool(component_ops ops);
    ~component_pool();

    component_pool(const component_pool&)                    = delete;
    auto operator=(const component_pool&) -> component_pool& = delete;
    component_pool(component_pool&& other) noexcept;
    auto operator=(component_pool&& other) noexcept -> component_pool&;

    auto emplace(entity e) -> void*;
    void remove(entity e);
    void batch_remove(const std::vector<entity>& entities);
    void clear();

    // Everything below sits on the iteration hot path, so it stays in the
    // interface where importers can inline it.
    [[nodiscard]] auto has(entity e) const -> bool {
        return e.index != entity::invalid_index && e.index < sparse_indices_.size() &&
            sparse_indices_[e.index] != entity::invalid_index &&
            dense_entities_[sparse_indices_[e.index]] == e;
    }

    [[nodiscard]] auto get(entity e) -> void* {
        return has(e) ? at(sparse_indices_[e.index]) : nullptr;
    }

    [[nodiscard]] auto get(entity e) const -> const void* {
        return has(e) ? at(sparse_indices_[e.index]) : nullptr;
    }

    // Callers that already established presence (a view, for one) skip the
    // second lookup.
    [[nodiscard]] auto get_unchecked(entity e) -> void* {
        return at(sparse_indices_[e.index]);
    }

    [[nodiscard]] auto at(uint32 dense_index) -> void* {
        return data_ + static_cast<std::size_t>(dense_index) * ops_.size;
    }

    [[nodiscard]] auto at(uint32 dense_index) const -> const void* {
        return data_ + static_cast<std::size_t>(dense_index) * ops_.size;
    }

    [[nodiscard]] auto size() const -> uint32 {
        return size_;
    }

    [[nodiscard]] auto ops() const -> const component_ops& {
        return ops_;
    }

    [[nodiscard]] auto entities() const -> const std::vector<entity>& {
        return dense_entities_;
    }

    [[nodiscard]] auto raw_data() -> void* {
        return data_;
    }

    [[nodiscard]] auto sparse() const -> std::span<const uint32> {
        return sparse_indices_;
    }

private:
    void reserve_(uint32 capacity);
    void grow_();

    component_ops ops_;
    std::byte* data_    = nullptr;
    uint32 size_        = 0;
    uint32 capacity_    = 0;
    std::vector<entity> dense_entities_;
    std::vector<uint32> sparse_indices_;
};

}  // namespace vw::ecs
