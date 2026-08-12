module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

void operation_manager::execute(
    std::unique_ptr<base_operation> op
) {
    op->execute();
    redo_.clear();
    undo_.emplace_back(std::move(op));
}

bool operation_manager::is_undo_empty() const {
    return undo_.empty();
}

void operation_manager::undo() {
    if (undo_.empty()) {
        return;
    }

    auto& op = undo_.back();
    op->undo();
    redo_.emplace_back(std::move(op));
    undo_.pop_back();
}

bool operation_manager::is_redo_empty() const {
    return redo_.empty();
}

void operation_manager::redo() {
    if (redo_.empty()) {
        return;
    }

    auto& op = redo_.back();
    op->execute();
    undo_.emplace_back(std::move(op));
    redo_.pop_back();
}

}  // namespace vw::sculptor
