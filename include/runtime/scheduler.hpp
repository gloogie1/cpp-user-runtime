#pragma once

#include "runtime/task.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace runtime {

class Task;

class Scheduler {
    public:
        void push(Task* task);
        void reserve(std::size_t capacity);
        Task* pop();
        
        bool empty() const;
        std::size_t size() const;

    private:
        struct ScheduledTask{
            Task* task;
            std::uint64_t sequence;
        };

        std::vector<ScheduledTask> heap_;
        std::uint64_t next_sequence_{0};

        bool higher_priority(const ScheduledTask& a, const ScheduledTask& b) const;
        void sift_up(std::size_t index);
        void sift_down(std::size_t index);

};

} // namespace runtime