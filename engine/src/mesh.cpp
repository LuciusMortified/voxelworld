#include <voxel/mesh.h>
#include <voxel/buffer.h>
#include <voxel/vulkan_context.h>

namespace voxel {

// ================== vertex ==================

std::vector<VkVertexInputBindingDescription> vertex::get_binding_descriptions() {
    std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].binding = 0;
    binding_descriptions[0].stride = sizeof(vertex);
    binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_descriptions;
}

std::vector<VkVertexInputAttributeDescription> vertex::get_attribute_descriptions() {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);
    
    // position
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(vertex, position);
    
    // normal
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(vertex, normal);
    
    // color
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_descriptions[2].offset = offsetof(vertex, color);
    
    return attribute_descriptions;
}


// ================== mesh ==================

mesh::mesh(std::shared_ptr<vulkan_context> context)
    : context_(context), vertex_buffer_(nullptr), index_buffer_(nullptr), 
      vertex_count_(0), index_count_(0) {
}

void mesh::set_vertices(const std::vector<vertex>& vertices) {
    vertex_count_ = vertices.size();
    if (vertex_count_ > 0) {
        vertex_buffer_ = std::make_unique<vertex_buffer>(context_, vertices);
    }
}

void mesh::set_indices(const std::vector<uint32>& indices) {
    index_count_ = indices.size();
    if (index_count_ > 0) {
        index_buffer_ = std::make_unique<index_buffer>(context_, indices);
    }
}

void mesh::set_mesh_data(const mesh_data& data) {
    set_vertices(data.vertices);
    set_indices(data.indices);
}

void mesh::bind(VkCommandBuffer command_buffer) {
    if (vertex_buffer_) {
        VkBuffer vertex_buffers[] = {vertex_buffer_->get_buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    }
    if (index_buffer_) {
        vkCmdBindIndexBuffer(command_buffer, index_buffer_->get_buffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

void mesh::draw(VkCommandBuffer command_buffer) {
    if (vertex_count_ > 0) {
        vkCmdDraw(command_buffer, static_cast<uint32>(vertex_count_), 1, 0, 0);
    }
}

void mesh::draw_indexed(VkCommandBuffer command_buffer) {
    if (index_count_ > 0) {
        vkCmdDrawIndexed(command_buffer, static_cast<uint32>(index_count_), 1, 0, 0, 0);
    }
}

// ================== simple_mesh_generator ==================

mesh simple_mesh_generator::generate_from_model(
    std::shared_ptr<vulkan_context> context,
    const std::shared_ptr<model>& model
) {
    if (!model) {
        return mesh(context);
    }
    
    mesh_data data = generate_mesh_data(model);
    
    mesh result(context);
    result.set_mesh_data(data);
    return result;
}

mesh_data simple_mesh_generator::generate_mesh_data(const std::shared_ptr<model>& model) {
    if (!model) {
        return mesh_data();
    }
    
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
    
    // Проходим по всем вокселам в модели
    for (int x = 0; x < model->width(); x++) {
        for (int y = 0; y < model->height(); y++) {
            for (int z = 0; z < model->depth(); z++) {
                voxel voxel_obj = model->get_voxel(x, y, z);
                if (voxel_obj.color != 0) { // Если воксел существует (не прозрачный)
                    vec3f position(x, y, z);
                    
                    // Проверяем каждую грань куба
                    for (int face = 0; face < 6; face++) {
                        if (is_face_visible(model, x, y, z, face)) {
                            add_cube_face(vertices, indices, position, face, voxel_obj.color);
                        }
                    }
                }
            }
        }
    }
    
    return mesh_data(std::move(vertices), std::move(indices));
}

void simple_mesh_generator::add_cube_face(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    const vec3f& position,
    int face_direction,
    uint32 color
) {
    // Направления граней: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    static const vec3f face_normals[6] = {
        vec3f(1, 0, 0),   // +X
        vec3f(-1, 0, 0),  // -X
        vec3f(0, 1, 0),   // +Y
        vec3f(0, -1, 0),  // -Y
        vec3f(0, 0, 1),   // +Z
        vec3f(0, 0, -1)   // -Z
    };
    
    // Вершины для каждой грани (4 вершины на грань)
    static const vec3f face_vertices[6][4] = {
        // +X face
        {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}},
        // -X face
        {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
        // +Y face
        {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},
        // -Y face
        {{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}},
        // +Z face
        {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},
        // -Z face
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}}
    };
    
    // Индексы для треугольника (2 треугольника на грань)
    static const uint32 face_indices[6] = {0, 1, 2, 2, 3, 0};
    
    uint32 base_vertex = static_cast<uint32>(vertices.size());
    vec3f normal = face_normals[face_direction];
    
    // Добавляем 4 вершины грани
    for (int i = 0; i < 4; i++) {
        vec3f vertex_pos = position + face_vertices[face_direction][i];
        vertices.emplace_back(vertex_pos, normal, color);
    }
    
    // Добавляем 6 индексов для двух треугольников
    for (int i = 0; i < 6; i++) {
        indices.push_back(base_vertex + face_indices[i]);
    }
}

bool simple_mesh_generator::is_face_visible(
    const std::shared_ptr<model>& model,
    int x,
    int y,
    int z,
    int face_direction
) {
    if (!model) return false;
    
    // Смещения для проверки соседних вокселов
    static const int dx[6] = {1, -1, 0, 0, 0, 0};
    static const int dy[6] = {0, 0, 1, -1, 0, 0};
    static const int dz[6] = {0, 0, 0, 0, 1, -1};
    
    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];
    
    // Проверяем границы модели
    if (nx < 0 || nx >= model->width() || 
        ny < 0 || ny >= model->height() || 
        nz < 0 || nz >= model->depth()) {
        return true; // Грань видна, если выходит за границы модели
    }
    
    // Грань видна, если соседний воксел не существует (прозрачный)
    return model->is_empty(nx, ny, nz);
}


// ================== greedy_mesh_generator ==================

mesh greedy_mesh_generator::generate_from_model(
    std::shared_ptr<vulkan_context> context,
    const std::shared_ptr<model>& model
) {
    if (!model) {
        return mesh(context);
    }
    
    mesh_data data = generate_mesh_data(model);
    
    mesh result(context);
    result.set_mesh_data(data);
    return result;
}

mesh_data greedy_mesh_generator::generate_mesh_data(const std::shared_ptr<model>& model) {
    if (!model) {
        return mesh_data();
    }
    
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
    
    // Генерируем грани для каждого направления
    for (int face_direction = 0; face_direction < 6; face_direction++) {
        generate_face_quads(vertices, indices, model, face_direction);
    }
    
    return mesh_data(std::move(vertices), std::move(indices));
}

void greedy_mesh_generator::generate_face_quads(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    const std::shared_ptr<model>& model, int face_direction
) {
    if (!model) return;
    
    // Определяем размеры и оси для текущего направления грани
    int width, height, depth;
    int axis1, axis2, axis3;
    
    switch (face_direction) {
        case 0: // +X
            width = model->depth(); height = model->height(); depth = model->width();
            axis1 = 2; axis2 = 1; axis3 = 0; break;
        case 1: // -X
            width = model->depth(); height = model->height(); depth = model->width();
            axis1 = 2; axis2 = 1; axis3 = 0; break;
        case 2: // +Y
            width = model->width(); height = model->depth(); depth = model->height();
            axis1 = 0; axis2 = 2; axis3 = 1; break;
        case 3: // -Y
            width = model->width(); height = model->depth(); depth = model->height();
            axis1 = 0; axis2 = 2; axis3 = 1; break;
        case 4: // +Z
            width = model->width(); height = model->height(); depth = model->depth();
            axis1 = 0; axis2 = 1; axis3 = 2; break;
        case 5: // -Z
            width = model->width(); height = model->height(); depth = model->depth();
            axis1 = 0; axis2 = 1; axis3 = 2; break;
    }
    
    // Проходим по каждому слою в направлении грани
    for (int layer = 0; layer < depth; layer++) {
        // Создаем маску видимых граней для текущего слоя
        std::vector<std::vector<uint32>> mask(width, std::vector<uint32>(height, 0));
        
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                // Преобразуем координаты обратно в координаты модели
                int mx, my, mz;
                switch (face_direction) {
                    case 0: // +X
                        mx = layer; my = y; mz = x; break;
                    case 1: // -X
                        mx = depth - 1 - layer; my = y; mz = x; break;
                    case 2: // +Y
                        mx = x; my = layer; mz = y; break;
                    case 3: // -Y
                        mx = x; my = depth - 1 - layer; mz = y; break;
                    case 4: // +Z
                        mx = x; my = y; mz = layer; break;
                    case 5: // -Z
                        mx = x; my = y; mz = depth - 1 - layer; break;
                }
                
                if (is_face_visible(model, mx, my, mz, face_direction)) {
                    mask[x][y] = model->get_voxel(mx, my, mz).color;
                }
            }
        }
        
        // Жадный алгоритм: объединяем соседние квадраты одного цвета
        std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
        
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                if (!visited[x][y] && mask[x][y] != 0) {
                    uint32 color = mask[x][y];
                    
                    // Находим максимальную ширину прямоугольника
                    int w = 1;
                    while (x + w < width && mask[x + w][y] == color && !visited[x + w][y]) {
                        w++;
                    }
                    
                    // Находим максимальную высоту прямоугольника
                    int h = 1;
                    bool can_extend = true;
                    while (can_extend && y + h < height) {
                        for (int i = 0; i < w; i++) {
                            if (mask[x + i][y + h] != color || visited[x + i][y + h]) {
                                can_extend = false;
                                break;
                            }
                        }
                        if (can_extend) h++;
                    }
                    
                    // Помечаем все квадраты в прямоугольнике как посещенные
                    for (int i = 0; i < w; i++) {
                        for (int j = 0; j < h; j++) {
                            visited[x + i][y + j] = true;
                        }
                    }
                    
                    // Преобразуем координаты обратно в координаты модели
                    vec3f min_pos, max_pos;
                    switch (face_direction) {
                        case 0: // +X
                            min_pos = vec3f(layer, y, x);
                            max_pos = vec3f(layer + 1, y + h, x + w);
                            break;
                        case 1: // -X
                            min_pos = vec3f(depth - 1 - layer, y, x);
                            max_pos = vec3f(depth - layer, y + h, x + w);
                            break;
                        case 2: // +Y
                            min_pos = vec3f(x, layer, y);
                            max_pos = vec3f(x + w, layer + 1, y + h);
                            break;
                        case 3: // -Y
                            min_pos = vec3f(x, depth - 1 - layer, y);
                            max_pos = vec3f(x + w, depth - layer, y + h);
                            break;
                        case 4: // +Z
                            min_pos = vec3f(x, y, layer);
                            max_pos = vec3f(x + w, y + h, layer + 1);
                            break;
                        case 5: // -Z
                            min_pos = vec3f(x, y, depth - 1 - layer);
                            max_pos = vec3f(x + w, y + h, depth - layer);
                            break;
                    }
                    
                    add_quad(vertices, indices, min_pos, max_pos, face_direction, color);
                }
            }
        }
    }
}

void greedy_mesh_generator::add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    const vec3f& min_pos,
    const vec3f& max_pos,
    int face_direction,
    uint32 color
) {
    // Направления граней: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    static const vec3f face_normals[6] = {
        vec3f(1, 0, 0),   // +X
        vec3f(-1, 0, 0),  // -X
        vec3f(0, 1, 0),   // +Y
        vec3f(0, -1, 0),  // -Y
        vec3f(0, 0, 1),   // +Z
        vec3f(0, 0, -1)   // -Z
    };
    
    // Вершины для каждого направления грани
    static const int vertex_indices[6][4] = {
        // +X: (max.x, min.y, min.z), (max.x, max.y, min.z), (max.x, max.y, max.z), (max.x, min.y, max.z)
        {0, 1, 2, 3},
        // -X: (min.x, min.y, max.z), (min.x, max.y, max.z), (min.x, max.y, min.z), (min.x, min.y, min.z)
        {3, 2, 1, 0},
        // +Y: (min.x, max.y, min.z), (max.x, max.y, min.z), (max.x, max.y, max.z), (min.x, max.y, max.z)
        {0, 1, 2, 3},
        // -Y: (min.x, min.y, max.z), (max.x, min.y, max.z), (max.x, min.y, min.z), (min.x, min.y, min.z)
        {3, 2, 1, 0},
        // +Z: (min.x, min.y, max.z), (min.x, max.y, max.z), (max.x, max.y, max.z), (max.x, min.y, max.z)
        {0, 1, 2, 3},
        // -Z: (max.x, min.y, min.z), (max.x, max.y, min.z), (min.x, max.y, min.z), (min.x, min.y, min.z)
        {0, 1, 2, 3}
    };
    
    // Создаем 8 вершин куба
    vec3f cube_vertices[8] = {
        vec3f(min_pos.x, min_pos.y, min_pos.z), // 0: (min, min, min)
        vec3f(max_pos.x, min_pos.y, min_pos.z), // 1: (max, min, min)
        vec3f(max_pos.x, max_pos.y, min_pos.z), // 2: (max, max, min)
        vec3f(min_pos.x, max_pos.y, min_pos.z), // 3: (min, max, min)
        vec3f(min_pos.x, min_pos.y, max_pos.z), // 4: (min, min, max)
        vec3f(max_pos.x, min_pos.y, max_pos.z), // 5: (max, min, max)
        vec3f(max_pos.x, max_pos.y, max_pos.z), // 6: (max, max, max)
        vec3f(min_pos.x, max_pos.y, max_pos.z)  // 7: (min, max, max)
    };
    
    uint32 base_vertex = static_cast<uint32>(vertices.size());
    vec3f normal = face_normals[face_direction];
    
    // Добавляем 4 вершины грани
    for (int i = 0; i < 4; i++) {
        int vertex_idx = vertex_indices[face_direction][i];
        vertices.emplace_back(cube_vertices[vertex_idx], normal, color);
    }
    
    // Добавляем 6 индексов для двух треугольников
    indices.push_back(base_vertex + 0);
    indices.push_back(base_vertex + 1);
    indices.push_back(base_vertex + 2);
    indices.push_back(base_vertex + 2);
    indices.push_back(base_vertex + 3);
    indices.push_back(base_vertex + 0);
}

bool greedy_mesh_generator::is_face_visible(const std::shared_ptr<model>& model, int x, int y, int z, int face_direction) {
    if (!model) return false;
    
    // Смещения для проверки соседних вокселов
    static const int dx[6] = {1, -1, 0, 0, 0, 0};
    static const int dy[6] = {0, 0, 1, -1, 0, 0};
    static const int dz[6] = {0, 0, 0, 0, 1, -1};
    
    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];
    
    // Проверяем границы модели
    if (nx < 0 || nx >= model->width() || 
        ny < 0 || ny >= model->height() || 
        nz < 0 || nz >= model->depth()) {
        return true; // Грань видна, если выходит за границы модели
    }
    
    // Грань видна, если соседний воксел не существует (прозрачный)
    return model->is_empty(nx, ny, nz);
}

} // namespace voxel 