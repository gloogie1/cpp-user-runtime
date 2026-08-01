#include "runtime/scheduler.hpp"
#include "runtime/task.hpp"

#include <utility>

namespace runtime {

void Scheduler::push(Task* task){
    ScheduledTask entry{task,next_sequence_};
    heap_.push_back(entry);
    next_sequence_++;
    sift_up(heap_.size()-1);
}

void Scheduler::reserve(std::size_t capacity){
    heap_.reserve(capacity);
}



Task* Scheduler::pop(){
    if(heap_.empty()){
        return nullptr;
    }
    if (heap_.size()==1){
        Task* t = heap_[0].task;
        heap_.pop_back();
        return t;
    }
    Task* t = heap_[0].task;
    heap_[0]=heap_.back();
    heap_.pop_back();
    sift_down(0);    
    return t;
}

bool Scheduler::empty() const{
    return heap_.empty();
}

std::size_t Scheduler::size() const{
    return heap_.size();
}

bool Scheduler::higher_priority(const ScheduledTask& a, const ScheduledTask& b) const{
    if(a.task->priority() > b.task->priority()){
        return true;
    }
    if(a.task->priority() < b.task->priority()){
        return false;
    }
    return a.sequence < b.sequence;
}

void Scheduler::sift_up(std::size_t index){
    while(index!=0 && higher_priority(heap_[index],heap_[(index-1)/2]) ){
        std::swap(heap_[index],heap_[(index-1)/2]);
        index = (index-1)/2;
    }
}

void Scheduler::sift_down(std::size_t index){
        
    while(true){   
        std::size_t best = index;
        std::size_t left = 2*index + 1;
        std::size_t right = 2*index + 2;
        std::size_t size = heap_.size();

        if(left < size){
            if(higher_priority(heap_[left], heap_[best])){
                best = left;
            }
        }

        if(right < size){
            if(higher_priority(heap_[right], heap_[best])){
                best = right;
            }
        }

        if (best == index) {
            break;
        }

        if(best != index){
            std::swap(heap_[index], heap_[best]);
            index=best;
        }
    }
}
} //namespace runtime