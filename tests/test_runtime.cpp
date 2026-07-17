#include "runtime/task_result.hpp"
#include "runtime/runtime.hpp"
#include "runtime/task_context.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>


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

    auto task_id = rt.spawn([&runs](runtime::TaskContext&){
        ++runs;
        if(runs == 1){
            return runtime::TaskResult::wait_for(std::chrono::milliseconds(0));
        }
        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(rt.tasks_completed(), 1);
    EXPECT_EQ(rt.tasks_failed(), 0);

    auto state = rt.task_state(task_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), runtime::TaskState::Completed);
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
    int runs = 0;

    runtime::TaskId id = rt.spawn([&runs](runtime::TaskContext&){
        ++runs;
        if(runs == 1){
            return runtime::TaskResult::wait_for(std::chrono::milliseconds(100));
        }
        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();
    auto state = rt.task_state(id);

    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(runs, 2);
    EXPECT_EQ(state.value(), runtime::TaskState::Completed);
    EXPECT_EQ(rt.tasks_completed(), 1);
    EXPECT_EQ(rt.tasks_failed(), 0);
}

TEST(RuntimeTest, InvalidTaskIdReturnsEmptyState){
    runtime::Runtime rt;

    auto state = rt.task_state(999);

    EXPECT_FALSE(state.has_value());
}

TEST(RuntimeTest, TasksExecuteByPriority){
    runtime::Runtime rt;

    std::vector<int> execution_order;

    rt.spawn([&execution_order](runtime::TaskContext&){
        execution_order.push_back(1);
        return runtime::TaskResult::complete();
    },5);

    rt.spawn([&execution_order](runtime::TaskContext&){
        execution_order.push_back(2);
        return runtime::TaskResult::complete();
    },1);

    rt.spawn([&execution_order](runtime::TaskContext&){
        execution_order.push_back(3);
        return runtime::TaskResult::complete();
    },10);

    rt.run_until_idle();

    EXPECT_EQ(execution_order, std::vector<int>({3, 1, 2}));
    EXPECT_EQ(rt.tasks_completed(), 3);
    EXPECT_EQ(rt.tasks_failed(), 0);
}


TEST(RuntimeTest, WaitingTaskRunsAgainAndCompletes){
    runtime::Runtime rt;
    int runs = 0;
    auto task_id = rt.spawn([&runs](runtime::TaskContext&){
        ++runs;
        if (runs == 1){
            return runtime::TaskResult::wait_for(std::chrono::milliseconds(0));
        }

        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();

    EXPECT_EQ(runs,2);
    EXPECT_EQ(rt.tasks_completed(), 1);
    EXPECT_EQ(rt.tasks_failed(), 0);
    
    auto state = rt.task_state(task_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), runtime::TaskState::Completed);
}

TEST(RuntimeTest, RunsReadyTaskWhileAnotherTaskWaits){
    runtime::Runtime rt;

    std::vector<std::string> execution_order;

    int a_runs = 0;
    rt.spawn([&a_runs, &execution_order](runtime::TaskContext&){
        execution_order.push_back("a"+std::to_string(++a_runs));
        if (a_runs == 1){
            return runtime::TaskResult::wait_for(std::chrono::milliseconds(10));
        }

        return runtime::TaskResult::complete();
    });
    
    rt.spawn([&execution_order](runtime::TaskContext&){
        execution_order.push_back("b");
        return runtime::TaskResult::complete();
    });
    rt.run_until_idle();

    const auto& metrics = rt.metrics();

    EXPECT_EQ(a_runs, 2);
    EXPECT_EQ(metrics.tasks_spawned, 2);
    EXPECT_EQ(metrics.task_invocations, 3);
    EXPECT_EQ(metrics.tasks_completed, 2);
    EXPECT_EQ(metrics.tasks_failed, 0);
    EXPECT_EQ(metrics.task_yields, 0);
    EXPECT_EQ(metrics.task_waits, 1);
    EXPECT_EQ(execution_order, std::vector<std::string>({"a1", "b", "a2"}));
}
