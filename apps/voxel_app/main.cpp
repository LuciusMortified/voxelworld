#include <iostream>

#include <voxel/engine.h>
#include <voxel/game_logic.h>
#include <voxel/voxel.h>
#include <voxel/model.h>
#include <voxel/world.h>
#include <voxel/math_utils.h>

// Игровая логика для воксельного приложения
class voxel_game_logic : public voxel::base_game_logic {
public:
    void initialize(std::shared_ptr<voxel::engine> engine) override {
        std::cout << "Инициализация воксельного приложения..." << std::endl;
        
        // Вызываем базовую инициализацию
        base_game_logic::initialize(engine);
        
        get_engine()->get_window()->on<voxel::events::key_press>([this](voxel::events::key_press& event) {
            handle_key_press(event.key);
            return event.handled;
        });
        
        get_engine()->get_window()->on<voxel::events::window_close>([this](voxel::events::window_close& event) {
            get_engine()->shutdown();
            return event.handled;
        });
        
        // Настройка движка
        get_engine()->get_renderer()->set_clear_color(0.1f, 0.2f, 0.3f, 1.0f);
        get_engine()->get_camera()->set_position({-5.0f, 0.0f, 0.0f});
        get_engine()->get_camera()->set_rotation(voxel::math::radians(0.0f), 0.0f);
        
        // Создание модели
        create_simple_cube();
        
        std::cout << "Воксельное приложение инициализировано!" << std::endl;
    }
    
    void update(float delta_time) override {
        // Вызываем базовую реализацию для обновления контроллера камеры
        base_game_logic::update(delta_time);
        
        // Вращение кубика
        update_cube_rotation(delta_time);
    }
    
    void render() override {
        // Дополнительный рендеринг можно добавить здесь
        // Основной рендеринг мира теперь выполняется в engine
    }
    
    void cleanup() override {
        std::cout << "Очистка воксельного приложения..." << std::endl;
    }
    
private:
    void create_simple_cube() {
        // Создаем простую кубическую модель 3x3x3
        cube_model_ = std::make_shared<voxel::model>(3, 3, 3);
        
        // Заполняем куб без перезаписи вокселов
        // Нижняя грань (синяя) - y=0
        cube_model_->set_voxel(0, 0, 0, voxel::BLUE);
        cube_model_->set_voxel(1, 0, 0, voxel::BLUE);
        cube_model_->set_voxel(2, 0, 0, voxel::BLUE);
        cube_model_->set_voxel(0, 0, 1, voxel::BLUE);
        cube_model_->set_voxel(1, 0, 1, voxel::BLUE);
        cube_model_->set_voxel(2, 0, 1, voxel::BLUE);
        cube_model_->set_voxel(0, 0, 2, voxel::BLUE);
        cube_model_->set_voxel(1, 0, 2, voxel::BLUE);
        cube_model_->set_voxel(2, 0, 2, voxel::BLUE);
        
        // Верхняя грань (зеленая) - y=2
        cube_model_->set_voxel(0, 2, 0, voxel::GREEN);
        cube_model_->set_voxel(1, 2, 0, voxel::GREEN);
        cube_model_->set_voxel(2, 2, 0, voxel::GREEN);
        cube_model_->set_voxel(0, 2, 1, voxel::GREEN);
        cube_model_->set_voxel(1, 2, 1, voxel::GREEN);
        cube_model_->set_voxel(2, 2, 1, voxel::GREEN);
        cube_model_->set_voxel(0, 2, 2, voxel::GREEN);
        cube_model_->set_voxel(1, 2, 2, voxel::GREEN);
        cube_model_->set_voxel(2, 2, 2, voxel::GREEN);
        
        // Передняя грань (красная) - z=0 (только средние вокселы)
        cube_model_->set_voxel(0, 1, 0, voxel::RED);
        cube_model_->set_voxel(1, 1, 0, voxel::RED);
        cube_model_->set_voxel(2, 1, 0, voxel::RED);
        
        // Задняя грань (желтая) - z=2 (только средние вокселы)
        cube_model_->set_voxel(0, 1, 2, voxel::YELLOW);
        cube_model_->set_voxel(1, 1, 2, voxel::YELLOW);
        cube_model_->set_voxel(2, 1, 2, voxel::YELLOW);
        
        // Левая грань (циановая) - x=0 (только средние вокселы)
        cube_model_->set_voxel(0, 1, 0, voxel::CYAN);
        cube_model_->set_voxel(0, 1, 1, voxel::CYAN);
        cube_model_->set_voxel(0, 1, 2, voxel::CYAN);
        
        // Правая грань (пурпурная) - x=2 (только средние вокселы)
        cube_model_->set_voxel(2, 1, 0, voxel::MAGENTA);
        cube_model_->set_voxel(2, 1, 1, voxel::MAGENTA);
        cube_model_->set_voxel(2, 1, 2, voxel::MAGENTA);
        
        // Центр куба (белый)
        cube_model_->set_voxel(1, 1, 1, voxel::WHITE);
        
        // Добавляем куб в мир движка
        cube_id_ = get_engine()->get_world()->add_object(cube_model_, {0.0f, 0.0f, 0.0f});
        std::cout << "Кубик добавлен в мир с ID: " << cube_id_ << std::endl;
        
        cube_rotation_ = 0.0f;
        cube_rotation_speed_ = voxel::math::radians(5.0f); // градусов в секунду
    }
    
    void update_cube_rotation(float delta_time) {
        cube_rotation_ += cube_rotation_speed_ * delta_time;
        
        // Обновляем поворот объекта в мире
        get_engine()->get_world()->set_object_rotation(cube_id_, {0.0f, cube_rotation_, 0.0f});
        
        // Выводим информацию о вращении
        static float last_print_time = 0.0f;
        last_print_time += delta_time;
        if (last_print_time >= 3.0f) {
            std::cout << "Кубик вращается: " << cube_rotation_ << "°" << std::endl;
            last_print_time = 0.0f;
        }
    }
    
    void handle_key_press(voxel::input::key key) {
        switch (key) {
            case voxel::input::key::ESCAPE:
                get_engine()->shutdown();
                break;
            case voxel::input::key::F1:
                if (camera_controller_) {
                    camera_controller_->toggle_cursor_mode();
                }
                break;
            case voxel::input::key::Q:
                std::cout << "Выход из приложения" << std::endl;
                get_engine()->shutdown();
                break;
            case voxel::input::key::KEY_1:
                // Изменение скорости вращения
                cube_rotation_speed_ = (cube_rotation_speed_ > 0) ? 0.0f : voxel::math::radians(5.0f);
                std::cout << "Скорость вращения: " << (cube_rotation_speed_ > 0 ? "включена" : "остановлена") << std::endl;
                break;
            default:
                break;
        }
    }
    
    std::shared_ptr<voxel::model> cube_model_;
    voxel::object_id cube_id_ = 0;
    float cube_rotation_{};
    float cube_rotation_speed_{};
};

int main() {
    try {
        std::cout << "Запуск Voxel App с вращающимся кубиком..." << std::endl;
        
        // Создание движка
        auto engine = std::make_shared<voxel::engine>(1280, 720, "Voxel App - Rotating Cube");
        
        // Создание и запуск игровой логики
        auto game_logic = std::make_unique<voxel_game_logic>();
        engine->run(std::move(game_logic));
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 