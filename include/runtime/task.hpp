#pragma once

#include "runtime/task_result.hpp"

#include <cstdint>
#include <functional>

namespace runtime {

class TaskContext;

using TaskId = std::uint64_t;
using TaskFunction = std::function<TaskResult(TaskContext&)>;

enum class TaskState {
    Ready,
    Running,
    Waiting,
    Completed,
    Cancelled,
    Failed
};

class Task {
public:
    Task(TaskId id, TaskFunction function);

    TaskId id() const;
    TaskState state() const;
    void set_state(TaskState state);

    TaskResult run(TaskContext& context);

private:
    TaskId id_;
    TaskState state_;
    TaskFunction function_;
};

} // namespace runtime
