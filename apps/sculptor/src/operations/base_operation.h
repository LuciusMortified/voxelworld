#pragma once

#ifndef VW_SCULPTOR_BASE_OPERATION_H
#define VW_SCULPTOR_BASE_OPERATION_H

namespace vw::sculptor {

class base_operation {
public:
    base_operation()          = default;
    virtual ~base_operation() = default;

    base_operation(base_operation&&)                    = default;
    auto operator=(base_operation&&) -> base_operation& = default;

    base_operation(const base_operation&)                    = delete;
    auto operator=(const base_operation&) -> base_operation& = delete;

    virtual void execute() = 0;
    virtual void undo()    = 0;
};

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_BASE_OPERATION_H
