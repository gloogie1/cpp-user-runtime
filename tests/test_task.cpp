#include "runtime/task_result.hpp"
#include "runtime/task.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

TEST(TaskResultTest, CompleteResultHasCompleteType) {
    const auto result = runtime::TaskResult::complete();

    EXPECT_EQ(result.type(), runtime::TaskResultType::Complete);
    EXPECT_EQ(result.wait_duration(), std::chrono::milliseconds(0));
    EXPECT_TRUE(result.error().empty());
}

TEST(TaskResultTest, YieldResultHasYieldType) {
    const auto result = runtime::TaskResult::yield();

    EXPECT_EQ(result.type(), runtime::TaskResultType::Yield);
    EXPECT_EQ(result.wait_duration(), std::chrono::milliseconds(0));
    EXPECT_TRUE(result.error().empty());
}

TEST(TaskResultTest, WaitResultStoresDuration) {
    const auto result = runtime::TaskResult::wait_for(std::chrono::milliseconds(250));

    EXPECT_EQ(result.type(), runtime::TaskResultType::Wait);
    EXPECT_EQ(result.wait_duration(), std::chrono::milliseconds(250));
    EXPECT_TRUE(result.error().empty());
}

TEST(TaskResultTest, FailResultStoresError) {
    const auto result = runtime::TaskResult::fail("task failed");

    EXPECT_EQ(result.type(), runtime::TaskResultType::Fail);
    EXPECT_EQ(result.wait_duration(), std::chrono::milliseconds(0));
    EXPECT_EQ(result.error(), "task failed");
}

TEST(TaskTest, NoPriorityArgumentSetsItToZero){ 
    runtime::Task task(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        }
    );
    
    EXPECT_EQ(task.priority(),0);
}

TEST(TaskTest, ExplicitArgumentSetsPriority){
    runtime::Task high_prio_task(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        10
    );
    runtime::Task low_prio_task(
        1,
        [](runtime::TaskContext&) {
            return runtime::TaskResult::complete();
        },
        5
    );
    
    EXPECT_EQ(high_prio_task.priority(), 10);
    EXPECT_EQ(low_prio_task.priority(), 5);
}