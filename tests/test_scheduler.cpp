#include "runtime/scheduler.hpp"
#include "runtime/task.hpp"
#include "runtime/task_context.hpp"
#include "runtime/task_result.hpp"

#include <gtest/gtest.h>

TEST(SchedulerTest, NewSchedulerIsEmpty){
    runtime::Scheduler schd;

    EXPECT_TRUE(schd.empty());
    EXPECT_EQ(schd.size(), 0);
}

TEST(SchedulerTest, PushingTaskIncreasesSize){
    runtime::Scheduler schd;

    runtime::Task task(
        1, 
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        }
    );

    schd.push(&task);

    EXPECT_FALSE(schd.empty());
    EXPECT_EQ(schd.size(), 1);
}

TEST(SchedulerTest, PopReturnsSameTaskPtr){
    runtime::Scheduler schd;

    runtime::Task task(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        }
    );

    schd.push(&task);
    runtime::Task* result_task = schd.pop();

    EXPECT_EQ(result_task, &task);
}

TEST(SchedulerTest, PopEmptyReturnsNullPtr){
    runtime::Scheduler schd;

    runtime::Task* result_task = schd.pop();

    EXPECT_EQ(result_task, nullptr);
    EXPECT_EQ(schd.size(), 0);
}

TEST(SchedulerTest, TasksPushPopInFifoOrder){
    runtime::Scheduler schd;

    runtime::Task task1(
        1,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        }
    );

    runtime::Task task2(
        2,
        [](runtime::TaskContext&){
            return runtime::TaskResult::complete();
        }
    );

    schd.push(&task1);
    schd.push(&task2);
    runtime::Task* result_task1 = schd.pop();
    runtime::Task* result_task2 = schd.pop();

    EXPECT_EQ(result_task1, &task1);
    EXPECT_EQ(result_task2, &task2);
    EXPECT_TRUE(schd.empty());
    EXPECT_EQ(schd.size(), 0);
}

TEST(SchedulerTest, HigherPriorityTasksPopFirst){
    runtime::Scheduler schd;
    
    runtime::Task low_prio_task(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        1
    );
    runtime::Task high_prio_task(
        2,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        10
    );

    runtime::Task mid_prio_task(
        3,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        5
    );

    schd.push(&low_prio_task);
    schd.push(&high_prio_task);
    schd.push(&mid_prio_task);

    runtime::Task* result_task1 = schd.pop();
    runtime::Task* result_task2 = schd.pop();
    runtime::Task* result_task3 = schd.pop();

    EXPECT_EQ(result_task1, &high_prio_task);
    EXPECT_EQ(result_task2, &mid_prio_task);
    EXPECT_EQ(result_task3, &low_prio_task);
    EXPECT_TRUE(schd.empty());
    EXPECT_EQ(schd.size(), 0);
}

TEST(SchedulerTest, EqualPriorityTasksPopInFifoOrder){
    runtime::Scheduler schd;
    
    runtime::Task task1(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        5
    );
    runtime::Task task2(
        2,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        5
    );

    runtime::Task task3(
        3,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        5
    );

    schd.push(&task1);
    schd.push(&task2);
    schd.push(&task3);

    runtime::Task* result_task1 = schd.pop();
    runtime::Task* result_task2 = schd.pop();
    runtime::Task* result_task3 = schd.pop();

    EXPECT_EQ(result_task1, &task1);
    EXPECT_EQ(result_task2, &task2);
    EXPECT_EQ(result_task3, &task3);
    EXPECT_TRUE(schd.empty());
    EXPECT_EQ(schd.size(), 0);
}
