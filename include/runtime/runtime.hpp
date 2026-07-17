#pragma once

#include "runtime/scheduler.hpp"
#include "runtime/task.hpp"
#include "runtime/task_result.hpp"
#include "runtime/timer_queue.hpp"
#include "runtime/runtime_metrics.hpp"

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>
#include <optional>

namespace runtime {

class Runtime {
public:
    Runtime();
    const RuntimeMetrics& metrics() const;

    TaskId spawn(TaskFunction function, int priority=0);
    void run_until_idle();

    std::uint64_t tasks_completed() const;
    std::uint64_t tasks_failed() const;
    std::optional<TaskState> task_state(TaskId id) const;
    
private:
    using Clock = TimerQueue::Clock;
    using TimePoint = TimerQueue::TimePoint;

    void wake_expired_tasks(TimePoint now);

    RuntimeMetrics metrics_;

    TaskId next_task_id_{1};

    std::vector<std::unique_ptr<Task>> tasks_;
    runtime::Scheduler scheduler_;
    TimerQueue timer_queue_;

};

} // namespace runtime
