export module vw.ecs:pool;

import std;

import vw.core;
import :entity;

export namespace vw::ecs {

// Разреженное множество без той части, что знает тип компонента: путь
// «сущность -> плотный индекс», обратный список владельцев и правила обмена с
// последним одинаковы у всех пулов. Двигать сами компоненты умеет только
// наследник — он единственный, кому их тип известен.
//
// Удаление меняет элемент местами с последним, поэтому порядок в плотном массиве
// не определён, зато обход остаётся непрерывным.
class pool_base {
public:
    pool_base()                                    = default;
    virtual ~pool_base()                           = default;
    pool_base(const pool_base&)                    = delete;
    auto operator=(const pool_base&) -> pool_base& = delete;
    pool_base(pool_base&&)                         = delete;
    auto operator=(pool_base&&) -> pool_base&      = delete;

    virtual auto remove(entity e) -> void = 0;
    virtual auto clear() -> void          = 0;

    auto batch_remove(const std::vector<entity>& entities) -> void {
        for (auto e : entities) {
            remove(e);
        }
    }

    // Всё ниже лежит на горячем пути обхода, поэтому остаётся в интерфейсе, где
    // импортирующий может это встроить.
    [[nodiscard]] auto has(entity e) const -> bool {
        return e.index != entity::invalid_index && e.index < sparse_indices_.size() &&
            sparse_indices_[e.index] != entity::invalid_index &&
            dense_entities_[sparse_indices_[e.index]] == e;
    }

    [[nodiscard]] auto slot_of(entity e) const -> uint32 {
        return sparse_indices_[e.index];
    }

    [[nodiscard]] auto size() const -> uint32 {
        return static_cast<uint32>(dense_entities_.size());
    }

    [[nodiscard]] auto entities() const -> const std::vector<entity>& {
        return dense_entities_;
    }

    [[nodiscard]] auto sparse() const -> std::span<const uint32> {
        return sparse_indices_;
    }

protected:
    // Место сущности в плотном массиве. fresh — правда, если места ещё не было и
    // наследнику надо дописать компонент в конец; ложь — если сущность уже здесь,
    // и тогда он обязан заменить компонент в этом слоте.
    struct slot {
        uint32 index = entity::invalid_index;
        bool fresh   = false;
    };

    auto reserve_slot_(entity e) -> slot;

    // Слот, освобождаемый удалением, и слот последнего элемента, который встанет
    // на его место. Оба invalid_index, если сущности в пуле нет; равны друг другу,
    // если удаляется как раз последний.
    struct removal {
        uint32 index = entity::invalid_index;
        uint32 last  = entity::invalid_index;
    };

    auto release_slot_(entity e) -> removal;

    auto clear_slots_() -> void;

private:
    std::vector<entity> dense_entities_;
    std::vector<uint32> sparse_indices_;
};

// Хранилище компонентов одного типа. Плотный массив — обычный std::vector: он же
// и выделяет память, и выравнивает её, и переносит содержимое при росте.
template <typename T>
class component_pool final : public pool_base {
public:
    template <typename... Args>
    auto emplace(entity e, Args&&... args) -> T& {
        const auto [index, fresh] = reserve_slot_(e);

        if (fresh) {
            dense_.emplace_back(std::forward<Args>(args)...);
        } else {
            // Замена на месте, а не присваивание: компоненту дозволено не иметь
            // операторов присваивания вовсе.
            std::destroy_at(std::addressof(dense_[index]));
            std::construct_at(std::addressof(dense_[index]), std::forward<Args>(args)...);
        }

        return dense_[index];
    }

    auto remove(entity e) -> void override {
        const auto [index, last] = release_slot_(e);
        if (index == entity::invalid_index) [[unlikely]] {
            return;
        }

        if (index != last) {
            std::destroy_at(std::addressof(dense_[index]));
            std::construct_at(std::addressof(dense_[index]), std::move(dense_[last]));
        }

        dense_.pop_back();
    }

    auto clear() -> void override {
        clear_slots_();
        dense_.clear();
    }

    [[nodiscard]] auto get(entity e) -> T* {
        return has(e) ? std::addressof(dense_[slot_of(e)]) : nullptr;
    }

    [[nodiscard]] auto get(entity e) const -> const T* {
        return has(e) ? std::addressof(dense_[slot_of(e)]) : nullptr;
    }

    // Вызывающие, уже убедившиеся в наличии (например, представление), пропускают
    // второй поиск.
    [[nodiscard]] auto get_unchecked(entity e) -> T& {
        return dense_[slot_of(e)];
    }

    [[nodiscard]] auto at(uint32 index) -> T& {
        return dense_[index];
    }

    [[nodiscard]] auto at(uint32 index) const -> const T& {
        return dense_[index];
    }

    [[nodiscard]] auto data() -> T* {
        return dense_.data();
    }

private:
    std::vector<T> dense_;
};

// Компонент, чей тип не назван ни в одной единице трансляции: пул знает о нём
// только размер и выравнивание. Значение обязано быть тривиальным — байты и
// ничего кроме, — потому что уничтожить или перенести его иначе, чем копией этих
// байтов, пулу нечем. Выравнивание не может быть строже, чем у max_align_t.
//
// Движок этим путём не ходит: он существует ради компонентов, приходящих из
// рантайма, где типа на этапе компиляции нет вовсе.
struct component_layout {
    uint32 size  = 0;
    uint32 align = alignof(std::max_align_t);
};

class dynamic_pool final : public pool_base {
public:
    explicit dynamic_pool(component_layout layout) : layout_{layout} {}

    auto emplace(entity e) -> void* {
        const auto [index, fresh] = reserve_slot_(e);
        if (fresh) {
            data_.resize(data_.size() + layout_.size);
        }
        return at(index);
    }

    auto remove(entity e) -> void override {
        const auto [index, last] = release_slot_(e);
        if (index == entity::invalid_index) [[unlikely]] {
            return;
        }

        if (index != last) {
            std::memcpy(at(index), at(last), layout_.size);
        }

        data_.resize(data_.size() - layout_.size);
    }

    auto clear() -> void override {
        clear_slots_();
        data_.clear();
    }

    [[nodiscard]] auto get(entity e) -> void* {
        return has(e) ? at(slot_of(e)) : nullptr;
    }

    [[nodiscard]] auto get(entity e) const -> const void* {
        return has(e) ? at(slot_of(e)) : nullptr;
    }

    [[nodiscard]] auto at(uint32 index) -> void* {
        return data_.data() + (static_cast<std::size_t>(index) * layout_.size);
    }

    [[nodiscard]] auto at(uint32 index) const -> const void* {
        return data_.data() + (static_cast<std::size_t>(index) * layout_.size);
    }

    [[nodiscard]] auto layout() const -> const component_layout& {
        return layout_;
    }

private:
    component_layout layout_;
    std::vector<std::byte> data_;
};

}  // namespace vw::ecs
