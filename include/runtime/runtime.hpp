#pragma once

#include "runtime/object_pool.hpp"
#include "runtime/runtime_metrics.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/task.hpp"
#include "runtime/task_result.hpp"
#include "runtime/timer_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>


namespace runtime {

class Runtime {
public:
    explicit Runtime(std::size_t task_pool_capacity_bytes = 1024 * 1024);
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

    ObjectPool<Task> task_pool_;
    std::vector<Task*> tasks_;
    Scheduler scheduler_;
    TimerQueue timer_queue_;

};

} // namespace runtime
