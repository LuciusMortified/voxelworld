#pragma once

#ifndef VW_SCULPTOR_OPERATION_MANAGER_H
#define VW_SCULPTOR_OPERATION_MANAGER_H

#include <memory>
#include <deque>

#include "base_operation.h"

namespace vw::sculptor {

class operation_manager final {
public:
    operation_manager() = default;

    operation_manager(const operation_manager&)                    = delete;
    auto operator=(const operation_manager&) -> operation_manager& = delete;

    void execute(std::unique_ptr<base_operation> op);

    [[nodiscard]] bool is_undo_empty() const;
    void undo();

    [[nodiscard]] bool is_redo_empty() const;
    void redo();

private:
    std::deque<std::unique_ptr<base_operation>> undo_;
    std::deque<std::unique_ptr<base_operation>> redo_;
};

}  // namespace vw::sculptor

#include "operation_manager.inl.h"

#endif  // VW_SCULPTOR_OPERATION_MANAGER_H
