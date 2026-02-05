#pragma once

#ifndef VW_SCULPTOR_OPERATION_MANAGER_INL_H
#define VW_SCULPTOR_OPERATION_MANAGER_INL_H

namespace vw::sculptor {

inline void operation_manager::set_dirty_flag(bool* flag) {
    dirty_flag_ = flag;
}

inline void operation_manager::mark_dirty_() {
    if (dirty_flag_) {
        *dirty_flag_ = true;
    }
}

inline void operation_manager::execute(
    std::unique_ptr<base_operation> op
) {
    op->execute();
    redo_.clear();
    undo_.emplace_back(std::move(op));
    mark_dirty_();
}

inline bool operation_manager::is_undo_empty() const {
    return undo_.empty();
}

inline void operation_manager::undo() {
    if (undo_.empty()) {
        return;
    }

    auto& op = undo_.back();
    op->undo();
    redo_.emplace_back(std::move(op));
    undo_.pop_back();
    mark_dirty_();
}

inline bool operation_manager::is_redo_empty() const {
    return redo_.empty();
}

inline void operation_manager::redo() {
    if (redo_.empty()) {
        return;
    }

    auto& op = redo_.back();
    op->execute();
    undo_.emplace_back(std::move(op));
    redo_.pop_back();
    mark_dirty_();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_OPERATION_MANAGER_INL_H
