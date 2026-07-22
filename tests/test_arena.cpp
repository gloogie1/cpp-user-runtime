#include "runtime/arena.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <new>

TEST(ArenaTest, NewArenaReportsInitialState) {
    runtime::Arena arena(1024);

    EXPECT_EQ(arena.capacity(), 1024);
    EXPECT_EQ(arena.used(), 0);
    EXPECT_EQ(arena.remaining(), 1024);
}

TEST(ArenaTest, AllocateIncreasesUsedSpace) {
    runtime::Arena arena(1024);

    EXPECT_EQ(arena.used(), 0);
    EXPECT_EQ(arena.remaining(), 1024);

    arena.allocate(128, 1);

    EXPECT_EQ(arena.used(), 128);
    EXPECT_EQ(arena.remaining(), 896);
}

TEST(ArenaTest, ExactCapacityAllocationSucceeds) {
    runtime::Arena arena(1024);

    void* pointer = arena.allocate(1024, 1);

    EXPECT_NE(pointer, nullptr);
    EXPECT_EQ(arena.used(), 1024);
    EXPECT_EQ(arena.remaining(), 0);
}

TEST(ArenaTest, AllocationBeyondCapacityThrowsStdBadAlloc) {
    runtime::Arena arena(1024);

    EXPECT_THROW(
        arena.allocate(2048, alignof(std::max_align_t)),
        std::bad_alloc
    );
}

TEST(ArenaTest, FailedAllocationLeavesUsedUnchanged) {
    runtime::Arena arena(1024);

    arena.allocate(128, 1);

    ASSERT_EQ(arena.used(), 128);
    ASSERT_EQ(arena.remaining(), 896);

    EXPECT_THROW(
        arena.allocate(2048, alignof(std::max_align_t)),
        std::bad_alloc
    );

    EXPECT_EQ(arena.used(), 128);
    EXPECT_EQ(arena.remaining(), 896);
}

TEST(ArenaTest, ReturnedAddressRespectsAlignment) {
    runtime::Arena arena(1024);

    constexpr std::size_t alignment = alignof(double);

    void* pointer = arena.allocate(128, alignment);

    const std::uintptr_t address =
        reinterpret_cast<std::uintptr_t>(pointer);

    EXPECT_EQ(address % alignment, 0);
}

TEST(ArenaTest, TwoAllocationsDoNotOverlap) {
    runtime::Arena arena(1024);

    void* pointer1 = arena.allocate(16, 1);
    void* pointer2 = arena.allocate(8, 1);

    const std::uintptr_t address1 =
        reinterpret_cast<std::uintptr_t>(pointer1);

    const std::uintptr_t address2 =
        reinterpret_cast<std::uintptr_t>(pointer2);

    EXPECT_EQ(address2, address1 + 16);
}

TEST(ArenaTest, AlignmentPaddingIsReflectedInUsedSpace) {
    runtime::Arena arena(1024);

    void* pointer1 = arena.allocate(1, 1);
    void* pointer2 = arena.allocate(1, 8);

    const std::uintptr_t address1 =
        reinterpret_cast<std::uintptr_t>(pointer1);

    const std::uintptr_t address2 =
        reinterpret_cast<std::uintptr_t>(pointer2);

    const std::size_t second_offset =
        static_cast<std::size_t>(address2 - address1);

    EXPECT_EQ(arena.used(), second_offset + 1);
}

TEST(ArenaTest, ResetPermitsAllocatingAgain) {
    runtime::Arena arena(1024);

    void* pointer1 = arena.allocate(128, 1);

    ASSERT_EQ(arena.used(), 128);
    ASSERT_EQ(arena.remaining(), 896);

    arena.reset();

    EXPECT_EQ(arena.capacity(), 1024);
    EXPECT_EQ(arena.used(), 0);
    EXPECT_EQ(arena.remaining(), 1024);

    void* pointer2 = arena.allocate(128, 1);

    EXPECT_EQ(pointer2, pointer1);
    EXPECT_EQ(arena.used(), 128);
    EXPECT_EQ(arena.remaining(), 896);
}