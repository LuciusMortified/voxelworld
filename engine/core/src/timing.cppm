export module vw.core:timing;

import std;

import :types;

export namespace vw {

template <typename F>
auto measure_ms(F&& fn) -> float32 {
    using clock = std::chrono::high_resolution_clock;
    const auto start = clock::now();
    fn();
    return std::chrono::duration<float32>(clock::now() - start).count() * 1000.0F;
}

}  // namespace vw
