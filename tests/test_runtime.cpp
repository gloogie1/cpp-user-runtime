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
