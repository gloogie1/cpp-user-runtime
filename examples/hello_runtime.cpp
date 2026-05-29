#include "runtime/runtime.hpp"
#include "runtime/task_result.hpp"

#include <iostream>

int main() {
    runtime::Runtime rt;

    rt.spawn([](runtime::TaskContext&) {
        std::cout << "task 1 executed\n";
        return runtime::TaskResult::complete();
    });

    rt.spawn([](runtime::TaskContext&) {
        std::cout << "task 2 executed\n";
        return runtime::TaskResult::complete();
    });

    rt.run_until_idle();

    std::cout << "completed: " << rt.tasks_completed() << "\n";
    std::cout << "failed: " << rt.tasks_failed() << "\n";

    return 0;
}
