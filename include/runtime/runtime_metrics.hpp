#pragma once

#include <cstddef>

namespace runtime {

struct RuntimeMetrics {
    std::size_t tasks_spawned{0};
    std::size_t task_invocations{0};
    std::size_t tasks_completed{0};
    std::size_t tasks_failed{0};
    std::size_t task_yields{0};
    std::size_t task_waits{0};
};

} //namespace runtime