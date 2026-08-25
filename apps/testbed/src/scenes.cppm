export module vw.testbed:scenes;

// Собирает сцены стенда и таблицу, по которой их выбирает командная строка.
export import :scenes.terrain;
export import :scenes.voxel_edits;
export import :scenes.lamp_edits;
export import :scenes.standing_lights;
export import :scenes.blob_shadows;
export import :scenes.animated_crowd;

import std;

import vw.core;
import :args;
import :scene;

export namespace vw::testbed {

// Имя сцены и то, как её построить, — в одном месте. Добавить сцену теперь
// значит написать её файл и вписать сюда строку; раньше это была правка четырёх
// разных ветвлений плюс разбора опций.
[[nodiscard]] auto find_scene(std::string_view name, const arg_reader& args)
    -> std::optional<scene_factory>;

// Для сообщения об ошибке: раньше имя, не совпавшее ни с одним, молча давало
// облёт пустого рельефа, и опечатка была не ошибкой, а другой сценой.
[[nodiscard]] auto scene_names() -> std::vector<std::string_view>;

}  // namespace vw::testbed
