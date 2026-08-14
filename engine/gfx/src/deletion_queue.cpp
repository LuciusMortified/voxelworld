module vw.gfx;

import std;
import vw.core;
import :resource;

namespace vw::gfx {

auto deletion_queue::set_frame(uint64 frame) -> void {
    current_frame_ = frame;
}

auto deletion_queue::retire(std::unique_ptr<buffer> victim) -> void {
    if (victim == nullptr) {
        return;
    }
    entries_.push_back({.frame = current_frame_, .victim = std::move(victim)});
}

auto deletion_queue::collect(uint64 completed_frame) -> void {
    std::erase_if(entries_, [completed_frame](const entry& e) {
        return e.frame <= completed_frame;
    });
}

auto deletion_queue::pending_count() const -> uint32 {
    return static_cast<uint32>(entries_.size());
}

}  // namespace vw::gfx
