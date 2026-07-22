#include "runtime/arena.hpp"

#include <memory>
#include <new>

namespace runtime{
Arena::Arena(std::size_t capacity):
    storage_(std::make_unique<std::byte[]>(capacity)),
    capacity_(capacity){}



void* Arena::allocate(std::size_t bytes, std::size_t alignment){
    void* current = storage_.get() + offset_;
    std::size_t space = remaining();
    void* aligned = std::align(alignment, bytes, current, space);
    
    if(aligned==nullptr){
        throw std::bad_alloc{};
    }

    auto* aligned_bytes = static_cast<std::byte*>(aligned);

    const std::size_t aligned_offset = static_cast<std::size_t>(aligned_bytes - storage_.get());

    offset_ = aligned_offset + bytes;

    return aligned;
}


void Arena::reset() noexcept {
    offset_ = 0;
}

std::size_t Arena::capacity() const noexcept{
    return capacity_;
}

std::size_t Arena::used() const noexcept{
    return offset_;
}

std::size_t Arena::remaining() const noexcept{
    return capacity_ - offset_;
}



}//namespace runtime