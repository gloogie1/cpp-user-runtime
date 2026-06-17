#include "runtime/task_result.hpp"
#include "runtime/runtime.hpp"
#include "runtime/task_context.hpp"

#include <gtest/gtest.h>

#include <chrono>


TEST(RuntimeTest, YieldTaskRunsAgainUntilComplete){
    runtime::Runtime rt;
    
    int runs = 0;

    rt.spawn([&runs](runtime::TaskContext&){
        ++runs;

        if(runs < 3){
            return runtime::TaskResult::yield();
        }

        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();

    EXPECT_EQ(runs, 3);
    EXPECT_EQ(rt.tasks_completed(), 1);
    EXPECT_EQ(rt.tasks_failed(), 0);
}


TEST(RuntimeTest, FailedTaskIsCountedAndNotRescheduled){
    runtime::Runtime rt;
    
    int runs = 0;

    rt.spawn([&runs](runtime::TaskContext&){
        ++runs;

        return runtime::TaskResult::fail("task failed");
    });
    rt.run_until_idle();

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(rt.tasks_completed(), 0);
    EXPECT_EQ(rt.tasks_failed(), 1);
}

TEST(RuntimeTest, WaitingTaskIsNotCountedAsCompleted){
    runtime::Runtime rt;
    
    int runs = 0;

    rt.spawn([&runs](runtime::TaskContext&){
        ++runs;

        return runtime::TaskResult::wait_for(std::chrono::milliseconds{100});
    });
    rt.run_until_idle();

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(rt.tasks_completed(), 0);
    EXPECT_EQ(rt.tasks_failed(), 0);
}


TEST(RuntimeTest, TaskStateCompleted){
    runtime::Runtime rt;

    runtime::TaskId id = rt.spawn([](runtime::TaskContext&){

        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();
    auto state = rt.task_state(id);

    EXPECT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), runtime::TaskState::Completed);
}

TEST(RuntimeTest, TaskStateFail){
    runtime::Runtime rt;

    runtime::TaskId id = rt.spawn([](runtime::TaskContext&){

        return runtime::TaskResult::fail("Task failed");
    });
    rt.run_until_idle();
    auto state = rt.task_state(id);

    EXPECT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), runtime::TaskState::Failed);
}

TEST(RuntimeTest, TaskStateWaiting){
    runtime::Runtime rt;

    runtime::TaskId id = rt.spawn([](runtime::TaskContext&){

        return runtime::TaskResult::wait_for(std::chrono::milliseconds(100));
    });
    rt.run_until_idle();
    auto state = rt.task_state(id);

    EXPECT_TRUE(state.has_value());
    EXPECT_EQ(rt.task_state(id), runtime::TaskState::Waiting);
    EXPECT_EQ(rt.tasks_completed(), 0);
    EXPECT_EQ(rt.tasks_failed(), 0);
}

TEST(RuntimeTest, InvalidTaskIdReturnsEmptyState){
    runtime::Runtime rt;

    auto state = rt.task_state(999);

    EXPECT_FALSE(state.has_value());
}