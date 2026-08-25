export module vw.testbed:scenes.terrain;

import std;

import vw.core;
import :app;
import :args;
import :scene;

export namespace vw::testbed {

// Голый рельеф и ничего больше: сцена, которая ничего не строит и ничего не
// правит, — чтобы мерить сам мир, его стриминг и его отсечение.
//
// Раньше на её месте было четыре класса сцены, и различал их только путь
// камеры. Путь теперь называет --camera, поэтому любой из них можно пустить по
// любой сцене, а не по одному пустому рельефу.
class terrain_scene final : public scene {
public:
    terrain_scene(testbed_app& stand, const arg_reader& /*args*/) : scene{stand} {}

    [[nodiscard]] auto name() const -> std::string_view override {
        return "terrain";
    }
};

}  // namespace vw::testbed
