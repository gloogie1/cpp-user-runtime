#include "runtime/task_result.hpp"
#include "runtime/runtime.hpp"
#include "runtime/task_context.hpp"

#include <gtest/gtest.h>


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
