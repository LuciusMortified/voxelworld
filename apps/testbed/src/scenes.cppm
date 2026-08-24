export module vw.testbed:scenes;

// Собирает сцены стенда и таблицу, по которой их выбирает командная строка.
export import :scenes.traverse;
export import :scenes.dig;
export import :scenes.light;
export import :scenes.torches;
export import :scenes.blobs;
export import :scenes.crowd;

import std;

import vw.core;
import :options;
import :scene;

export namespace vw::testbed {

// Имя сцены и то, как её построить, — в одном месте. Добавить сцену теперь
// значит написать её файл и вписать сюда строку; раньше это была правка четырёх
// разных ветвлений плюс разбора опций.
[[nodiscard]] auto find_scene(std::string_view name, const testbed_options& opts)
    -> std::optional<scene_factory>;

// Для сообщения об ошибке: раньше имя, не совпавшее ни с одним, молча давало
// flythrough, и опечатка была не ошибкой, а другой сценой.
[[nodiscard]] auto scene_names() -> std::vector<std::string_view>;

}  // namespace vw::testbed
