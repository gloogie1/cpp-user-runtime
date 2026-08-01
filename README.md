# C++ User-Space Task Runtime

A single-threaded C++20 runtime for callback-based tasks with explicit lifecycle states, priority scheduling, delayed resumption, runtime metrics, and arena-backed task ownership.

The project implements the core mechanics of a small execution runtime: task representation, ready-queue ordering, timer management, object lifetime, aligned allocation, and exception-safe task allocation and construction. Tasks are regular C++ callbacks rather than stackful coroutines, so yielding or waiting causes the callback to be invoked again.

## Current Capabilities

- Callback-based tasks using `std::function<TaskResult(TaskContext&)>`
- Explicit task states and result-driven transitions
- Custom binary-heap priority scheduler
- FIFO tie-breaking between equal-priority tasks
- Delayed task resumption using `std::chrono::steady_clock`
- Execution of ready tasks while other tasks are waiting
- Task lookup by `TaskId`
- Runtime counters for spawning, invocation, completion, failure, yielding, and waiting
- Fixed-capacity aligned arena allocator
- Typed `ObjectPool<T>` with placement construction and reverse-order destruction
- Arena-backed ownership of runtime `Task` objects
- GoogleTest coverage for tasks, scheduling, timers, runtime behavior, allocation, object lifetime, and constructor-failure rollback
## Architecture Overview

```text
                              Runtime
                                 |
        +----------------+-------+-------+----------------+
        |                |               |                |
        v                v               v                v
 ObjectPool<Task>   vector<Task*>     Scheduler       TimerQueue
 owns Task objects  lookup registry   ready Task*     waiting Task*

                    All Task* values are non-owning



The main components have separate responsibilities:

- `Runtime` coordinates task execution and state transitions.
- `ObjectPool<Task>` owns and destroys all spawned task objects.
- `std::vector<Task*>` indexes tasks for `TaskId` lookup.
- `Scheduler` orders ready tasks using priority and insertion sequence.
- `TimerQueue` holds delayed tasks until their wake time.
- `RuntimeMetrics` records execution counters.
- `TaskContext` is currently an empty execution-context placeholder.

## Task Lifecycle and `TaskResult`

A newly constructed task starts in `Ready`. Before callback execution, it enters `Running`. Its returned `TaskResult` determines the next state.

| Result | State transition | Runtime behavior |
|---|---|---|
| `Complete` | `Running -> Completed` | Records one completed task |
| `Yield` | `Running -> Ready` | Reinserts the task into the scheduler |
| `Wait` | `Running -> Waiting` | Adds the task to the timer queue |
| `Fail` | `Running -> Failed` | Records one failed task |

The task-state enum also contains `Cancelled`, but cancellation behavior is not implemented.

A callback that yields or waits is invoked again from the beginning. Continuation state must therefore be stored explicitly, such as in lambda captures or another object referenced by the callback.

`TaskResult::fail()` stores an error string in the result. The current runtime marks the task as failed and updates metrics, but does not retain or expose that error after processing the result.

## Priority Scheduler

`Scheduler` is a custom binary max-heap backed by `std::vector`.

Each heap entry contains:

```text
Task*          non-owning pointer to a task
sequence       insertion sequence assigned by the scheduler
```

Ordering follows two rules:

1. Higher numeric priority runs first.
2. Equal-priority entries run in FIFO insertion order.

The scheduler assigns a new sequence whenever a task is pushed. A task that yields or wakes from the timer queue therefore re-enters the ready queue as a new entry behind already queued tasks of the same priority.

Insertion uses sift-up. Removal replaces the root with the final heap entry and restores ordering with iterative sift-down.

## Timer Queue and Delayed Tasks

`TaskResult::wait_for(duration)` delays a task using an absolute `std::chrono::steady_clock` wake time.

Each timer entry contains:

```text
Task*                         non-owning task pointer
steady_clock::time_point      wake time
```

The current timer queue is vector-based:

- `add()` appends a timer entry.
- `pop_ready(now)` scans all entries and removes tasks whose wake time is at or before `now`.
- Future entries remain in the queue.
- `next_wake_time()` scans for the earliest deadline.

At the beginning of each runtime loop iteration, expired tasks are marked `Ready` and moved back into the scheduler.

When no task is ready but delayed tasks remain, the runtime calls `std::this_thread::sleep_until()` for the earliest wake time. It does not sleep while another task is ready to execute.

## Runtime Task Ownership

The runtime uses the following ownership model:

```text
ObjectPool<Task>    owns and destroys Task objects
vector<Task*>       indexes tasks for TaskId lookup
Scheduler           holds non-owning Task pointers
TimerQueue          holds non-owning Task pointers
```

Neither the lookup vector, scheduler, nor timer queue deletes tasks.

The members are declared so that C++ reverse destruction order destroys the timer queue, scheduler, and lookup vector before `ObjectPool<Task>` destroys the task objects they reference.

The task pool uses a configurable byte capacity with a default of one MiB:

```cpp
runtime::Runtime runtime;
runtime::Runtime custom_runtime(256 * 1024);
```

Tasks are retained for the lifetime of the runtime. Completed and failed tasks are not individually reclaimed.

## Arena Allocator and `ObjectPool`

### Arena

`Arena` is a fixed-capacity bump allocator backed by a contiguous byte array.

It supports:

- Aligned raw-memory allocation through `std::align`
- Capacity, used-byte, and remaining-byte inspection
- Whole-arena reset
- Rewinding to an earlier valid offset
- `std::bad_alloc` when an allocation cannot fit

The allocator advances its offset only after alignment succeeds. Alignment padding contributes to used capacity.

Arena allocations cannot be freed individually.

### ObjectPool

`ObjectPool<T>` adds typed object lifetime management over `Arena`.

It supports:

- Perfect-forwarded construction with `std::construct_at`
- Tracking of successfully constructed objects
- Reverse-order destruction with `std::destroy_at`
- Automatic destruction of live objects when the pool is destroyed
- Bulk reset and storage reuse
- Fixed-capacity exhaustion through `std::bad_alloc`

The object pool is a bulk-lifetime arena owner, not a free-list allocator. Individual objects cannot currently be removed or reused independently.

Its pointer-tracking vector uses ordinary `std::vector` storage; the objects themselves are constructed inside the arena.

## Exception-Safety Guarantees

### Arena allocation

The implementation and tests verify that:

- An allocation that cannot fit throws `std::bad_alloc`.
- A failed allocation leaves the arena offset unchanged.
- Reset restores the complete arena capacity.
- Rewind restores an earlier allocation position.

### Object construction

Before arena allocation, `ObjectPool<T>::create()` ensures that its live-object tracking vector has spare capacity.

It then records the current arena offset before attempting construction. If the constructor throws:

1. The arena rewinds to its previous position.
2. The failed object is not added to the live-object list.
3. Existing objects remain valid and tracked.
4. The original exception is rethrown.
5. The reclaimed storage can be used by a later successful construction.

Tests cover constructor failure both in an empty pool and after an existing object has already been created.

Tests also verify reverse-order destruction, destructor execution during pool destruction, repeated reset safety, capacity exhaustion, and storage reuse after reset.

### `Runtime::spawn()`

`Runtime::spawn()` is structured to provide the strong exception guarantee for failures during storage preparation or `Task` construction:

1. The task lookup vector reserves space.
2. The scheduler reserves corresponding heap space.
3. The next task ID is read but not incremented.
4. The task is constructed in `ObjectPool<Task>`.
5. The non-owning pointer is added to the lookup vector and scheduler.
6. The task ID and `tasks_spawned` metric are incremented.

Because lookup and scheduler storage are reserved before task construction, their subsequent pointer insertions do not require reallocation.

A failed reserve or task construction therefore does not:

- Register a partial task
- Consume a `TaskId`
- Increment `tasks_spawned`
- Change the logical task or scheduler contents

The current test suite does not contain a targeted failure-injection test for this runtime-level guarantee.

## Runtime Metrics

`RuntimeMetrics` exposes the following counters:

```text
tasks_spawned
task_invocations
tasks_completed
tasks_failed
task_yields
task_waits
```

Metrics are available through:

```cpp
const runtime::RuntimeMetrics& metrics = runtime.metrics();
```

`task_invocations` counts callback executions rather than unique tasks. A task that waits and later completes contributes two invocations, one wait, and one completion.

## Build Instructions

### Requirements

- CMake 3.20 or newer
- A compiler with C++20 support
- A CMake-supported build system
- Network access during the initial configuration if GoogleTest 1.14.0 is not already cached

### Configure and build

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

To use Ninja explicitly:

```bash
cmake -S . -B build/debug \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build/debug
```

### Run the example

```bash
./build/debug/hello_runtime
```

The example spawns two completing tasks, runs the runtime until idle, and prints completion and failure totals.

## Test Instructions

Tests use GoogleTest 1.14.0, obtained through CMake `FetchContent`, and are registered with CTest.

```bash
ctest --test-dir build/debug --output-on-failure
```

To rebuild and rerun after a code change:

```bash
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
```

The tests cover:

- `TaskResult` types, wait durations, and failure messages
- Default and explicit task priorities
- Empty, insertion, removal, priority, and FIFO scheduler behavior
- Timer expiration, future-task retention, and earliest-wake lookup
- Yield, wait, repeat invocation, completion, and failure behavior
- Priority-based runtime execution
- Execution of ready work while another task is delayed
- Task-state lookup and invalid task IDs
- Runtime metric accounting
- Arena capacity, alignment, exhaustion, reset, and accounting
- Object construction, destruction, reverse destruction, reset, and reuse
- Object-pool exhaustion and constructor-failure rollback

## Repository Structure

```text
.
├── .gitignore
├── CMakeLists.txt
├── README.md
├── benchmarks/
│   └── CMakeLists.txt
├── docs/
│   └── architecture.md
├── examples/
│   └── hello_runtime.cpp
├── include/
│   └── runtime/
│       ├── arena.hpp
│       ├── object_pool.hpp
│       ├── runtime.hpp
│       ├── runtime_metrics.hpp
│       ├── scheduler.hpp
│       ├── task.hpp
│       ├── task_context.hpp
│       ├── task_result.hpp
│       └── timer_queue.hpp
├── src/
│   ├── arena.cpp
│   ├── runtime.cpp
│   ├── scheduler.cpp
│   ├── task.cpp
│   └── timer_queue.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_arena.cpp
    ├── test_object_pool.cpp
    ├── test_runtime.cpp
    ├── test_scheduler.cpp
    ├── test_task.cpp
    └── test_timer_queue.cpp
```

## Current Limitations

- Execution is single-threaded, and the runtime is not thread-safe.
- Worker threads, work stealing, cancellation, and task reclamation are not implemented.
- Tasks are callback-driven state machines rather than stackful or coroutine-based continuations.
- Completed and failed tasks remain allocated until runtime destruction.
- The timer queue uses linear scans rather than a deadline heap.
- `run_until_idle()` blocks while delayed tasks remain and cannot bound indefinitely yielding tasks.
- Exceptions thrown directly by callbacks are not yet translated into task failures.
- No benchmarks, sanitizer presets, or profiling configurations are currently included.

## Planned Next Steps

- Add failure-injection tests for `Runtime::spawn()` and define callback-exception behavior.
- Add controlled task reclamation and cancellation semantics.
- Replace the vector-scanned timer queue with a deadline-oriented structure.
- Add sanitizer configurations, profiling, and focused benchmarks.
- Introduce worker threads and work stealing after preserving the current ownership and scheduling invariants.