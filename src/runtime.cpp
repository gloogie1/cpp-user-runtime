#include "runtime/runtime.hpp"

#include "runtime/task_context.hpp"

#include <memory>
#include <thread>
#include <utility>

namespace runtime {

Runtime::Runtime() = default;

const RuntimeMetrics& Runtime::metrics() const {
    return metrics_;
}

TaskId Runtime::spawn(TaskFunction function, int priority) {
    const TaskId id = next_task_id_++;

    auto task = std::make_unique<Task>(id, std::move(function), priority);
    Task* task_ptr = task.get();

    tasks_.push_back(std::move(task));
    scheduler_.push(task_ptr);
    ++metrics_.tasks_spawned;

    return id;
}



void Runtime::run_until_idle() {
    TaskContext context;

    while (!scheduler_.empty() || !timer_queue_.empty()) {
        wake_expired_tasks(Clock::now());

        if(scheduler_.empty()){
            auto next_wake = timer_queue_.next_wake_time();

            if(next_wake.has_value()){
                std::this_thread::sleep_until(*next_wake);
            }

            continue;
        }
        
        Task* task = scheduler_.pop();

        task->set_state(TaskState::Running);
        ++metrics_.task_invocations;
        TaskResult result = task->run(context);

        switch (result.type()) {
            case TaskResultType::Complete:
                task->set_state(TaskState::Completed);
                ++metrics_.tasks_completed;
                break;

            case TaskResultType::Yield:
                task->set_state(TaskState::Ready);
                scheduler_.push(task);
                ++metrics_.task_yields;
                break;

            case TaskResultType::Wait: {
                task->set_state(TaskState::Waiting);
                auto wake_time = Clock::now() + result.wait_duration();
                timer_queue_.add(task, wake_time);
                ++metrics_.task_waits;
                break;
            }
            case TaskResultType::Fail:
                task->set_state(TaskState::Failed);
                ++metrics_.tasks_failed;
                break;
        }
    }
}

std::size_t Runtime::tasks_completed() const {
    return metrics_.tasks_completed;
}

std::size_t Runtime::tasks_failed() const {
    return metrics_.tasks_failed;
}


void Runtime::wake_expired_tasks(TimePoint now) {
    auto ready_tasks = timer_queue_.pop_ready(now);

    for (Task* task : ready_tasks) {
        task->set_state(TaskState::Ready);
        scheduler_.push(task);
    }
}

std::optional<TaskState> Runtime::task_state(TaskId id) const {
    for(const auto& task: tasks_){
        if(task->id() == id){
            return task->state();
        }
    }
    return std::nullopt;
}

} // namespace runtime
