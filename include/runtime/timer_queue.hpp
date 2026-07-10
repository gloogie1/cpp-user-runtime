#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace runtime {

class Task;

class TimerQueue {
public:

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void add(Task* task, TimePoint wake_time);
    std::vector<Task*> pop_ready(TimePoint now);

    bool empty() const;
    std::size_t size() const;

    std::optional<TimePoint> next_wake_time() const;

private:
    struct TimerEntry {
        Task* task;
        TimePoint wake_time;
    };

    std::vector<TimerEntry> entries_;
};
} // namespace runtime