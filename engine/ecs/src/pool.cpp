module vw.ecs;

import std;

namespace vw::ecs {
namespace {

auto allocate(std::size_t bytes, uint32 align) -> std::byte* {
    if (bytes == 0) {
        return nullptr;
    }
    return static_cast<std::byte*>(::operator new(bytes, std::align_val_t{align}));
}

void deallocate(std::byte* data, uint32 align) {
    if (data != nullptr) {
        ::operator delete(data, std::align_val_t{align});
    }
}

}  // namespace

component_pool::component_pool(component_ops ops) : ops_{ops} {}

component_pool::~component_pool() {
    clear();
    deallocate(data_, ops_.align);
}

component_pool::component_pool(component_pool&& other) noexcept
    : ops_{other.ops_},
      data_{std::exchange(other.data_, nullptr)},
      size_{std::exchange(other.size_, 0)},
      capacity_{std::exchange(other.capacity_, 0)},
      dense_entities_{std::move(other.dense_entities_)},
      sparse_indices_{std::move(other.sparse_indices_)} {}

auto component_pool::operator=(component_pool&& other) noexcept -> component_pool& {
    if (this != &other) {
        clear();
        deallocate(data_, ops_.align);

        ops_            = other.ops_;
        data_           = std::exchange(other.data_, nullptr);
        size_           = std::exchange(other.size_, 0);
        capacity_       = std::exchange(other.capacity_, 0);
        dense_entities_ = std::move(other.dense_entities_);
        sparse_indices_ = std::move(other.sparse_indices_);
    }
    return *this;
}

auto component_pool::emplace(entity e) -> void* {
    if (e.index >= sparse_indices_.size()) [[unlikely]] {
        sparse_indices_.resize(e.index + 1, entity::invalid_index);
    }

    if (has(e)) [[unlikely]] {
        void* slot = at(sparse_indices_[e.index]);
        if (ops_.destroy != nullptr) {
            ops_.destroy(slot);
        }
        return slot;
    }

    if (size_ == capacity_) {
        grow_();
    }

    const uint32 index = size_++;
    dense_entities_.push_back(e);
    sparse_indices_[e.index] = index;
    return at(index);
}

void component_pool::remove(entity e) {
    if (!has(e)) [[unlikely]] {
        return;
    }

    const uint32 dense_index = sparse_indices_[e.index];
    const uint32 last_index  = size_ - 1;

    if (ops_.destroy != nullptr) {
        ops_.destroy(at(dense_index));
    }

    if (dense_index != last_index) {
        ops_.relocate(at(dense_index), at(last_index));
        dense_entities_[dense_index]                       = dense_entities_[last_index];
        sparse_indices_[dense_entities_[dense_index].index] = dense_index;
    }

    dense_entities_.pop_back();
    --size_;
    sparse_indices_[e.index] = entity::invalid_index;
}

void component_pool::batch_remove(const std::vector<entity>& entities) {
    for (auto e : entities) {
        remove(e);
    }
}

void component_pool::clear() {
    if (ops_.destroy != nullptr) {
        for (uint32 i = 0; i < size_; ++i) {
            ops_.destroy(at(i));
        }
    }
    size_ = 0;
    dense_entities_.clear();
    std::ranges::fill(sparse_indices_, entity::invalid_index);
}

void component_pool::reserve_(uint32 capacity) {
    if (capacity <= capacity_) {
        return;
    }

    std::byte* fresh = allocate(static_cast<std::size_t>(capacity) * ops_.size, ops_.align);
    for (uint32 i = 0; i < size_; ++i) {
        ops_.relocate(fresh + static_cast<std::size_t>(i) * ops_.size, at(i));
    }

    deallocate(data_, ops_.align);
    data_     = fresh;
    capacity_ = capacity;
    dense_entities_.reserve(capacity);
}

void component_pool::grow_() {
    constexpr uint32 initial_capacity = 8;
    reserve_(capacity_ == 0 ? initial_capacity : capacity_ * 2);
}

}  // namespace vw::ecs
