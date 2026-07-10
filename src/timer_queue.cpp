#include "runtime/timer_queue.hpp"

#include <utility>

namespace runtime {

void TimerQueue::add(Task* task, TimePoint wake_time) {
    TimerEntry entry{task, wake_time};
    entries_.push_back(entry);
}

std::vector<Task*> TimerQueue::pop_ready(TimePoint now) {
    std::vector<Task*> ready_tasks;
    std::vector<TimerEntry> still_waiting;

    for (const auto& entry : entries_) {
        if (entry.wake_time <= now) {
            ready_tasks.push_back(entry.task);
        } else {
            still_waiting.push_back(entry);
        }
    }

    entries_ = std::move(still_waiting);

    return ready_tasks;
}

bool TimerQueue::empty() const {
    return entries_.empty();
}

std::size_t TimerQueue::size() const {
    return entries_.size();
}

std::optional<TimerQueue::TimePoint> TimerQueue::next_wake_time() const {
    if (entries_.empty()) {
        return std::nullopt;
    }

    TimePoint earliest = entries_[0].wake_time;

    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].wake_time < earliest) {
            earliest = entries_[i].wake_time;
        }
    }

    return earliest;
}

} // namespace runtime