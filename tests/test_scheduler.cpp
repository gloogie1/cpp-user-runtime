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