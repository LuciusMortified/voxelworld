export module vw.world:model;

// Собирает воксельную модель: идентичность, страницы, занятость граней, связи
// полостей и поле света.
export import :model.identity;
export import :model.occupancy;
export import :model.links;
export import :model.light_field;

import std;

import vw.core;

export namespace vw::asset {

// Страничный воксельный объём. Страницы бывают пустыми, однородными и
// разреженными, поэтому почти сплошная или почти пустая модель стоит одной записи
// на страницу вместо массива вокселей.
class model {
public:
    static constexpr int32 page_size   = 8;
    static constexpr int32 page_volume = page_size * page_size * page_size;
    using page_type                    = std::array<voxel, page_volume>;

    model(model_identity_pool& identity_pool, page_pool& pool, int32 width, int32 height,
          int32 depth, int32 voxel_scale = 1);
    ~model();

    model(const model&)                    = delete;
    auto operator=(const model&) -> model& = delete;

    model(model&& other) noexcept;
    auto operator=(model&& other) noexcept -> model&;

    auto set_voxel(int32 x, int32 y, int32 z, const voxel& v) -> void;

    // Та же запись, но без увеличения поколения. set_voxel берёт мьютекс пула
    // идентичностей на каждый вызов: для правки это нормально, для генерации
    // разорительно — чанк это десятки тысяч записей на четырёх воркерах.
    // Заполнению свежей модели инвалидация не нужна вовсе, её ещё никто не мешил;
    // изменение живой зовёт invalidate() один раз в конце.
    auto set_voxel_raw(int32 x, int32 y, int32 z, const voxel& v) -> void;

    auto set_voxel(vec3i pos, const voxel& v) -> void {
        set_voxel(pos.x, pos.y, pos.z, v);
    }

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel {
        const auto& entry = pages_[page_index(x / page_size, y / page_size, z / page_size)];

        switch (entry.mode()) {
            case page_mode::empty:
                return voxel{};
            case page_mode::uniform:
                return voxel{entry.fill_id()};
            case page_mode::sparse:
                return pool_ptr_->get(entry.pool_index())
                    [local_index(x % page_size, y % page_size, z % page_size)];
        }
        return voxel{};
    }

    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel {
        return get_voxel(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool {
        const auto& entry = pages_[page_index(x / page_size, y / page_size, z / page_size)];

        switch (entry.mode()) {
            case page_mode::empty:
                return true;
            case page_mode::uniform:
                return false;
            case page_mode::sparse:
                return pool_ptr_->get(entry.pool_index())
                    [local_index(x % page_size, y % page_size, z % page_size)]
                        .is_empty();
        }
        return true;
    }

    [[nodiscard]] auto is_empty(vec3i pos) const -> bool {
        return is_empty(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] auto width() const -> int32 {
        return width_;
    }

    [[nodiscard]] auto height() const -> int32 {
        return height_;
    }

    [[nodiscard]] auto depth() const -> int32 {
        return depth_;
    }

    [[nodiscard]] auto size() const -> vec3i {
        return vec3i{width_, height_, depth_};
    }

    [[nodiscard]] auto voxel_scale() const -> int32 {
        return voxel_scale_;
    }

    // Записанная страница остаётся разреженной, даже если все воксели в ней
    // оказались одним блоком, — поэтому сплошная порода стоит 512 байт на
    // страницу вместо нуля. Сворачивание таких обратно в однородные и делает
    // глубокий мир подъёмным; звать один раз, когда объём заполнен.
    auto compact_pages() -> uint32;

    // Собственная плоскость модели со стороны face_direction, прямо из таблицы
    // страниц: однородная страница — это восемь строк по восемь выставленных бит,
    // и вокселей она не касается. Ложь для всего, что не 64-куб: соседей, которым
    // нужна плоскость, имеют только чанки мира.
    [[nodiscard]] auto extract_face(int32 face_direction, face_occupancy& out) const -> bool;

    // Идёт по записям страниц, а не по вокселям: чанк сплошной породы — это 512
    // однородных записей, и ответ выходит из первых двух различающихся. Звать
    // после compact_pages: разреженная страница, где случайно один блок, читается
    // как смешанная.
    //
    // Кэшируется, потому что важный вызывающий — сосед, спрашивающий через шов, а
    // к тому моменту таблица страниц холодная: два килобайта промахов кэша ради
    // одного байта. Стоит прогреть на потоке, сгенерировавшем чанк.
    [[nodiscard]] auto scan_fill() const -> model_fill;

    // Все соседние плоскости известны и в каждой выставлены все биты, то есть
    // снаружи этой модели не видно ничего. Отсутствующий срез — открытое небо, и
    // считается не сплошным.
    [[nodiscard]] auto boundaries_are_solid() const -> bool;

    // Заполняет битовый объём из таблицы страниц, а не воксель за вокселем: пустая
    // страница не даёт ничего, однородная даёт восемь выставленных бит на строку
    // разом. Ложь, если модель не 64-куб.
    [[nodiscard]] auto build_occupancy(chunk_occupancy& out) const -> bool;

    // Только строки вдоль X, не трогая zrows, и только на заданном диапазоне
    // страниц: x в [px0 * 8, px1 * 8) и z в [pz0 * 8, pz1 * 8). Строки вне
    // диапазона по z обнуляются, а не заполняются.
    //
    // Диапазон и делает небесный свет подъёмным. Построить все девять колонок
    // окрестности целиком стоит около четырнадцати миллисекунд, впятеро дороже
    // заливки, которую это кормит, и почти всё — цена касания каждого вокселя
    // восьми чанков. Но юбка — пятнадцать вокселей, то есть две страницы по
    // восемь: боковой сосед читается на две страничные колонки, угловой на две на
    // две, и вся окрестность выходит в 2,25 колонки работы вместо девяти.
    [[nodiscard]] auto build_x_rows(chunk_occupancy& out, int32 px0, int32 px1, int32 pz0,
                                    int32 pz1) const -> bool;

    [[nodiscard]] auto build_x_rows(chunk_occupancy& out) const -> bool {
        return build_x_rows(out, 0, pages_x_, 0, pages_z_);
    }

    auto set_boundary_slice(int32 face_direction, const model& neighbor) -> void;

    // Верно только пока has_boundary_slice это подтверждает.
    [[nodiscard]] auto get_boundary_face(int32 face_direction) const -> const face_occupancy& {
        return boundary_->faces[face_direction];
    }

    [[nodiscard]] auto has_boundary_slice(int32 face_direction) const -> bool {
        return boundary_ != nullptr && (boundary_->valid & (1U << face_direction)) != 0;
    }

    // Зовётся, когда меш построен. Всё, ради чего плоскости были нужны, к этому
    // моменту уже случилось, а держать их стоит трёх килобайт на чанк всё время,
    // пока чанк загружен.
    auto release_boundary() -> void;

    [[nodiscard]] auto is_boundary_solid(int32 face_direction, int32 x, int32 y, int32 z) const
        -> bool;

    auto invalidate() -> void;

    // Небесный свет этого чанка либо ничего, если он ещё не освещён. Держится так
    // же, как плоскости границ, за указателем, потому что он бывает только у
    // чанков мира: над моделью в редакторе неба нет. Три килобайта, когда он есть.
    //
    // Правка оставляет его на месте, а не сбрасывает. Устаревший свет на кадр, за
    // который идёт перезаливка, не виден вовсе; отсутствие света выглядит так,
    // будто чанк почернел.
    auto set_sky_light(light_field light) -> void;

    [[nodiscard]] auto get_sky_light() const -> const light_field* {
        return light_.get();
    }

    [[nodiscard]] auto has_sky_light() const -> bool {
        return light_ != nullptr;
    }

    // Другой канал, хранится так же и по той же причине. Держится отдельно от
    // первого и никогда с ним не суммируется: небесный свет — это видимость,
    // которую умножает время суток, а свет блоков — свет, которого оно трогать не
    // должно. В сумме лампа гасла бы к вечеру.
    auto set_block_light(light_field light) -> void;

    [[nodiscard]] auto get_block_light() const -> const light_field* {
        return block_light_.get();
    }

    [[nodiscard]] auto has_block_light() const -> bool {
        return block_light_ != nullptr;
    }

    auto fill(const voxel& v) -> void;

    // Целая страница одной записью. Рельеф ниже поверхности — один и тот же блок
    // на сотни вокселей подряд, и запись по вокселю стоит и цикла, и разреженной
    // страницы, которую пул потом сворачивает обратно.
    auto fill_page_raw(int32 px, int32 py, int32 pz, const voxel& v) -> void;

    [[nodiscard]] auto get_identity() const -> model_identity {
        return identity_;
    }

    auto clone_pages_from(const model& source) -> void;

    [[nodiscard]] auto get_page_mode(int32 px, int32 py, int32 pz) const -> page_mode {
        return pages_[page_index(px, py, pz)].mode();
    }

    [[nodiscard]] auto get_page_fill_id(int32 px, int32 py, int32 pz) const -> block_id {
        return pages_[page_index(px, py, pz)].fill_id();
    }

    [[nodiscard]] auto get_page(int32 px, int32 py, int32 pz) const -> const page_type* {
        const auto& entry = pages_[page_index(px, py, pz)];
        return entry.mode() == page_mode::sparse ? &pool_ptr_->get(entry.pool_index()) : nullptr;
    }

    [[nodiscard]] auto pages_x() const -> int32 {
        return pages_x_;
    }

    [[nodiscard]] auto pages_y() const -> int32 {
        return pages_y_;
    }

    [[nodiscard]] auto pages_z() const -> int32 {
        return pages_z_;
    }

private:
    [[nodiscard]] auto page_index(int32 px, int32 py, int32 pz) const -> int32 {
        return px + (py * pages_x_) + (pz * pages_x_ * pages_y_);
    }

    [[nodiscard]] static auto local_index(int32 lx, int32 ly, int32 lz) -> int32 {
        return lx + (ly * page_size) + (lz * page_size * page_size);
    }

    auto alloc_sparse_page() -> uint32;
    auto free_sparse_page(uint32 index) -> void;
    auto promote_to_sparse(int32 px, int32 py, int32 pz) -> page_type&;

    auto increment_generation_() -> void;

    model_identity_pool* identity_pool_;
    page_pool* pool_ptr_;
    int32 width_{0}, height_{0}, depth_{0};
    int32 voxel_scale_{1};
    int32 pages_x_{0}, pages_y_{0}, pages_z_{0};
    std::vector<page_entry> pages_;
    std::vector<uint32> owned_pages_;
    model_identity identity_;
    std::unique_ptr<model_boundary> boundary_;
    std::unique_ptr<light_field> light_;
    std::unique_ptr<light_field> block_light_;
    mutable model_fill fill_ = model_fill::mixed;
    mutable bool fill_known_ = false;
};

// Владеет именованными моделями мира вместе с пулами, из которых берутся их
// страницы и идентичности.
class model_registry {
public:
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<model>;

    [[nodiscard]] auto create(std::string_view name, int32 width, int32 height, int32 depth)
        -> std::shared_ptr<model>;
    [[nodiscard]] auto create(std::string_view name, vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(int32 width, int32 height, int32 depth)
        -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_clone(std::string_view name) -> std::shared_ptr<model>;

    auto erase(std::string_view name) -> void;

    [[nodiscard]] auto get_identity_pool() -> model_identity_pool& {
        return identity_pool_;
    }

    [[nodiscard]] auto get_page_pool() -> page_pool& {
        return page_pool_;
    }

private:
    model_identity_pool identity_pool_;
    page_pool page_pool_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
};

}  // namespace vw::asset
