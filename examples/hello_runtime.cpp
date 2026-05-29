#include "runtime/task.hpp"
#include "runtime/task_context.hpp"
#include "runtime/task_result.hpp"

#include <iostream>

int main() {
    runtime::TaskContext context;

    runtime::Task task(
        1,
        [](runtime::TaskContext&) {
            std::cout << "task executed\n";
            return runtime::TaskResult::complete();
        }
    );

    auto result = task.run(context);

    if (result.type() == runtime::TaskResultType::Complete) {
        std::cout << "task completed\n";
    }

    return 0;
}
