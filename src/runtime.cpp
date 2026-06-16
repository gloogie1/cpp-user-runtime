#include "runtime/runtime.hpp"

#include "runtime/task_context.hpp"

#include <memory>
#include <utility>

namespace runtime {

Runtime::Runtime()
    : next_task_id_(1),
      tasks_completed_(0),
      tasks_failed_(0) {}

TaskId Runtime::spawn(TaskFunction function) {
    const TaskId id = next_task_id_++;

    auto task = std::make_unique<Task>(id, std::move(function));
    Task* task_ptr = task.get();

    tasks_.push_back(std::move(task));
    ready_queue_.push(task_ptr);

    return id;
}

void Runtime::run_until_idle() {
    TaskContext context;

    while (!ready_queue_.empty()) {
        Task* task = ready_queue_.front();
        ready_queue_.pop();

        TaskResult result = task->run(context);

        switch (result.type()) {
            case TaskResultType::Complete:
                task->set_state(TaskState::Completed);
                ++tasks_completed_;
                break;

            case TaskResultType::Yield:
                task->set_state(TaskState::Ready);
                ready_queue_.push(task);
                break;

            case TaskResultType::Wait:
                task->set_state(TaskState::Waiting);
                // Timer queue comes later. For now, treat wait as completed placeholder.
                break;

            case TaskResultType::Fail:
                task->set_state(TaskState::Failed);
                ++tasks_failed_;
                break;
        }
    }
}

std::uint64_t Runtime::tasks_completed() const {
    return tasks_completed_;
}

std::uint64_t Runtime::tasks_failed() const {
    return tasks_failed_;
}

} // namespace runtime
