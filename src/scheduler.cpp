#include "runtime/scheduler.hpp"

namespace runtime {

    void Scheduler::push(Task* task){
        ready_queue_.push(task);
    }

    Task* Scheduler::pop(){
        if(ready_queue_.empty()){
            return nullptr;
        }
        Task* t=ready_queue_.front();    
        ready_queue_.pop();
        return t;
    }

    bool Scheduler::empty() const{
        return ready_queue_.empty();
    }

    std::size_t Scheduler::size() const{
        return ready_queue_.size();
    }    
} //namespace runtime