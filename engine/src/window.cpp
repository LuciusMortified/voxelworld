#include <voxel/window.h>
#include <stdexcept>
#include <vector>

namespace voxel {

window::window(int width, int height, std::string_view title) 
    : width_(width), height_(height) {
    
    // Инициализация GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    // Настройка GLFW для работы с Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Создание окна
    window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    
    // Настройка обработчиков событий
    glfwSetWindowUserPointer(window_, this);
    
    // Установка GLFW callbacks
    glfwSetKeyCallback(window_, key_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetCursorPosCallback(window_, mouse_motion_callback);
    glfwSetScrollCallback(window_, mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, window_size_callback);
    glfwSetWindowFocusCallback(window_, window_focus_callback);
    glfwSetWindowCloseCallback(window_, window_close_callback);
}

window::~window() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

// GLFW callback функции
void window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    
    // Преобразование GLFW mods в input::mod
    input::mod input_mods = static_cast<input::mod>(mods);
    
    // Создание соответствующего события в зависимости от action
    switch (action) {
        case GLFW_PRESS: {
            events::key_press event(static_cast<input::key>(key), scancode, input_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
        case GLFW_RELEASE: {
            events::key_release event(static_cast<input::key>(key), scancode, input_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
        case GLFW_REPEAT: {
            events::key_repeat event(static_cast<input::key>(key), scancode, input_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
    }
}

void window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    
    // Преобразование GLFW mods в input::mod
    input::mod input_mods = static_cast<input::mod>(mods);
    
    // Создание соответствующего события в зависимости от action
    if (action == GLFW_PRESS) {
        events::mouse_press event(static_cast<input::mouse_button>(button), input_mods);
        w->event_dispatcher_.dispatch(event);
    } else if (action == GLFW_RELEASE) {
        events::mouse_release event(static_cast<input::mouse_button>(button), input_mods);
        w->event_dispatcher_.dispatch(event);
    }
}

void window::mouse_motion_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    w->last_cursor_x_ = xpos;
    w->last_cursor_y_ = ypos;
    
    events::mouse_move event(xpos, ypos);
    w->event_dispatcher_.dispatch(event);
}

void window::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    
    events::mouse_scroll event(xoffset, yoffset);
    w->event_dispatcher_.dispatch(event);
}

void window::window_size_callback(GLFWwindow* window, int width, int height) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    w->width_ = width;
    w->height_ = height;
    
    events::window_resize event(width, height);
    w->event_dispatcher_.dispatch(event);
}

void window::window_focus_callback(GLFWwindow* window, int focused) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    
    events::window_focus event(focused == GLFW_TRUE);
    w->event_dispatcher_.dispatch(event);
}

void window::window_close_callback(GLFWwindow* window) {
    auto* w = static_cast<voxel::window*>(glfwGetWindowUserPointer(window));
    
    events::window_close event;
    w->event_dispatcher_.dispatch(event);
}

bool window::should_close() const {
    return glfwWindowShouldClose(window_);
}

void window::poll_events() {
    glfwPollEvents();
}

void window::get_framebuffer_size(int* width, int* height) const {
    glfwGetFramebufferSize(window_, width, height);
}

VkSurfaceKHR window::create_surface(VkInstance instance) {
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
    
    return surface;
}

std::vector<const char*> window::get_required_extensions() const {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    
    // Добавляем дополнительные расширения, если нужно
    #ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    #endif
    
    return extensions;
}

bool window::is_key_pressed(input::key key) const {
    return glfwGetKey(window_, static_cast<int>(key)) == GLFW_PRESS;
}

bool window::is_mouse_button_pressed(input::mouse_button button) const {
    return glfwGetMouseButton(window_, static_cast<int>(button)) == GLFW_PRESS;
}

void window::get_cursor_pos(double* xpos, double* ypos) const {
    glfwGetCursorPos(window_, xpos, ypos);
}

void window::set_cursor_pos(double xpos, double ypos) {
    glfwSetCursorPos(window_, xpos, ypos);
    last_cursor_x_ = xpos;
    last_cursor_y_ = ypos;
}

void window::set_cursor_mode(input::cursor_mode mode) {
    glfwSetInputMode(window_, GLFW_CURSOR, static_cast<int>(mode));
}

void window::set_input_mode(input::input_mode mode, int value) {
    glfwSetInputMode(window_, static_cast<int>(mode), value);
}

void window::set_title(std::string_view title) {
    glfwSetWindowTitle(window_, title.data());
}

void window::set_size(int width, int height) {
    glfwSetWindowSize(window_, width, height);
    width_ = width;
    height_ = height;
}

void window::set_position(int xpos, int ypos) {
    glfwSetWindowPos(window_, xpos, ypos);
}

void window::maximize() {
    glfwMaximizeWindow(window_);
}

void window::minimize() {
    glfwIconifyWindow(window_);
}

void window::restore() {
    glfwRestoreWindow(window_);
}

} // namespace voxel 