#include "runtime/timer_queue.hpp"
#include "runtime/task.hpp"
#include "runtime/task_context.hpp"
#include "runtime/task_result.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>


TEST(TimerQueueTest, TimerQueueIsEmpty){
    runtime::TimerQueue tmq;

    EXPECT_TRUE(tmq.empty());
    EXPECT_EQ(tmq.size(), 0);
}

TEST(TimerQueueTest, TimerQueueSizeIncrease){
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();

    runtime::Task task(
        1, 
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task, now);

    EXPECT_FALSE(tmq.empty());
    EXPECT_EQ(tmq.size(), 1);
}

TEST(TimerQueueTest, DoesNotPopFutureTask){
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();
    auto future = now + std::chrono::milliseconds(100);

    runtime::Task task(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task, future);
    auto result_tasks = tmq.pop_ready(now);

    EXPECT_TRUE(result_tasks.empty());
    EXPECT_EQ(tmq.size(), 1);
    EXPECT_FALSE(tmq.empty());
}

TEST(TimerQueueTest, PopsTasksDueNowAndEarlier){
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();
    auto past = now - std::chrono::milliseconds(100);

    runtime::Task task1(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task2(
        2,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task1, now);
    tmq.add(&task2, past);

    auto result_tasks = tmq.pop_ready(now);
    
    EXPECT_EQ(result_tasks.size(), 2);
    EXPECT_EQ(tmq.size(), 0);
    EXPECT_TRUE(tmq.empty());
    EXPECT_NE(
        std::find(result_tasks.begin(), result_tasks.end(), &task1),
        result_tasks.end()
    );
    EXPECT_NE(
        std::find(result_tasks.begin(), result_tasks.end(), &task2),
        result_tasks.end()
    );
}

TEST(TimerQueueTest, LeavesFutureTasksInQueue){
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();
    auto past = now - std::chrono::milliseconds(100);
    auto future = now + std::chrono::milliseconds(100);

    runtime::Task task1(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task2(
        2,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task3(
        3,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task1, now);
    tmq.add(&task2, past);
    tmq.add(&task3, future);

    auto result_tasks = tmq.pop_ready(now);

    ASSERT_EQ(result_tasks.size(), 2);
    EXPECT_EQ(tmq.size(), 1);
    EXPECT_FALSE(tmq.empty());
    
    EXPECT_NE(
        std::find(result_tasks.begin(), result_tasks.end(), &task1),
        result_tasks.end()
    );

    EXPECT_NE(
        std::find(result_tasks.begin(), result_tasks.end(), &task2),
        result_tasks.end()
    );
    
    auto later_tasks = tmq.pop_ready(future);
    
    ASSERT_EQ(later_tasks.size(), 1);
    EXPECT_EQ(later_tasks[0], &task3);
    EXPECT_TRUE(tmq.empty());
    EXPECT_EQ(tmq.size(), 0);
}

TEST(TimerQueueTest, NextWakeTimeReturnsEarliest){
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();
    auto past = now - std::chrono::milliseconds(100);
    auto future = now + std::chrono::milliseconds(100);

    runtime::Task task1(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task2(
        2,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task3(
        3,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task1, now);
    tmq.add(&task2, past);
    tmq.add(&task3, future);

    auto earliest = tmq.next_wake_time();

    ASSERT_TRUE(earliest.has_value());
    EXPECT_EQ(earliest.value(), past);
}

TEST(TimerQueueTest, NextWakeTimeUpdatesAfterReadyTaskIsPopped) {
    runtime::TimerQueue tmq;

    auto now = runtime::TimerQueue::Clock::now();
    auto first_wake = now;
    auto second_wake = now + std::chrono::milliseconds(100);

    runtime::Task task1(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task task2(
        2,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        10
    );

    tmq.add(&task1, first_wake);
    tmq.add(&task2, second_wake);

    auto ready_tasks = tmq.pop_ready(now);

    ASSERT_EQ(ready_tasks.size(), 1);
    EXPECT_EQ(ready_tasks[0], &task1);

    auto next_wake = tmq.next_wake_time();

    ASSERT_TRUE(next_wake.has_value());
    EXPECT_EQ(next_wake.value(), second_wake);
}



TEST(TimerQueueTest, EmptyQueueHasNoNextWakeTime) {
    runtime::TimerQueue tmq;

    auto next_wake_time = tmq.next_wake_time();

    EXPECT_FALSE(next_wake_time.has_value());
}
