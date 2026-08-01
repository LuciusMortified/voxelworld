import std;
import vw.core.spike;

auto main() -> int {
    const vw::vec3f v{3.0F, 4.0F, 0.0F};
    std::println("vw.core.spike: |{}| = {}", vw::describe(v), vw::length(v));
    return vw::length(v) == 5.0F ? 0 : 1;
}
