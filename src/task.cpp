#include "runtime/task.hpp"
#include "runtime/task_result.hpp"

#include <utility>

namespace runtime {

TaskResult TaskResult::complete() {
    return TaskResult(TaskResultType::Complete, std::chrono::milliseconds(0), "");
}

TaskResult TaskResult::yield() {
    return TaskResult(TaskResultType::Yield, std::chrono::milliseconds(0), "");
}

TaskResult TaskResult::wait_for(std::chrono::milliseconds duration) {
    return TaskResult(TaskResultType::Wait, duration, "");
}

TaskResult TaskResult::fail(std::string error) {
    return TaskResult(TaskResultType::Fail, std::chrono::milliseconds(0), std::move(error));
}

TaskResultType TaskResult::type() const {
    return type_;
}

std::chrono::milliseconds TaskResult::wait_duration() const {
    return wait_duration_;
}

const std::string& TaskResult::error() const {
    return error_;
}

TaskResult::TaskResult(
    TaskResultType type,
    std::chrono::milliseconds wait_duration,
    std::string error
)
    : type_(type),
      wait_duration_(wait_duration),
      error_(std::move(error)) {}

Task::Task(TaskId id, TaskFunction function)
    : id_(id),
      state_(TaskState::Ready),
      function_(std::move(function)) {}

TaskId Task::id() const {
    return id_;
}

TaskState Task::state() const {
    return state_;
}

void Task::set_state(TaskState state) {
    state_ = state;
}

TaskResult Task::run(TaskContext& context) {
    state_ = TaskState::Running;
    return function_(context);
}

} // namespace runtime
