#pragma once

#include "runtime/task.hpp"

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>
#include <optional>

namespace runtime {

class Runtime {
public:
    Runtime();

    TaskId spawn(TaskFunction function);
    void run_until_idle();

    std::uint64_t tasks_completed() const;
    std::uint64_t tasks_failed() const;
    std::optional<TaskState> task_state(TaskId id) const;
    
private:
    TaskId next_task_id_;
    std::vector<std::unique_ptr<Task>> tasks_;
    std::queue<Task*> ready_queue_;

    std::uint64_t tasks_completed_;
    std::uint64_t tasks_failed_;
};

} // namespace runtime
