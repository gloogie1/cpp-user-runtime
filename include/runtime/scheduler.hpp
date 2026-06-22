#pragma once

#include <cstddef>
#include <queue>

namespace runtime {

class Task;

class Scheduler {
    public:
        void push(Task* task);
        Task* pop();
        bool empty() const;

        std::size_t size() const;

    private:
        std::queue<Task*> ready_queue_;
};
}