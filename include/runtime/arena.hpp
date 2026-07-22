#pragma once

#include <cstddef>
#include <memory>

namespace runtime {

class Arena {
public:
    explicit Arena(std::size_t capacity);

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    void* allocate(std::size_t bytes, std::size_t alignment);

    void reset() noexcept;

    std::size_t capacity() const noexcept;
    std::size_t used() const noexcept;
    std::size_t remaining() const noexcept;

private:
    std::unique_ptr<std::byte[]> storage_;
    std::size_t capacity_;
    std::size_t offset_{0};

};


}//namespace runtime