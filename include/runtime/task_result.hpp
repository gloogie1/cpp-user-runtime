#pragma once

#include <chrono>
#include <string>

namespace runtime {

enum class TaskResultType {
    Complete,
    Yield,
    Wait,
    Fail
};

class TaskResult {
public:
    static TaskResult complete();
    static TaskResult yield();
    static TaskResult wait_for(std::chrono::milliseconds duration);
    static TaskResult fail(std::string error);

    TaskResultType type() const;
    std::chrono::milliseconds wait_duration() const;
    const std::string& error() const;

private:
    TaskResult(
        TaskResultType type,
        std::chrono::milliseconds wait_duration,
        std::string error
    );

    TaskResultType type_;
    std::chrono::milliseconds wait_duration_;
    std::string error_;
};

} // namespace runtime
