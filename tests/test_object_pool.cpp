#include "runtime/object_pool.hpp"

#include <gtest/gtest.h>

#include <vector>
#include <stdexcept>

namespace {

struct TrackedObject {
    TrackedObject(int value, int& constructions, int& destructions)
        : value(value),
            destructions(&destructions){
                ++constructions;
            }
    ~TrackedObject(){
        ++(*destructions);
    }

    int value;
    int* destructions;
};

struct OrderedTrackedObject {
    OrderedTrackedObject(
        int value,
        std::vector<int>& destruction_order
    )
        : value(value),
          destruction_order(&destruction_order) {
    }

    ~OrderedTrackedObject() {
        destruction_order->push_back(value);
    }

    int value;
    std::vector<int>* destruction_order;
};


struct ThrowingObject {
    ThrowingObject(int value, bool should_throw)
        : value(value) {
        if (should_throw) {
            throw std::runtime_error("constructor failed");
        }
    }

    int value;
};


} // namespace


TEST(ObjectPoolTest, NewPoolHasZeroObjects) {
    runtime::ObjectPool<int> pool(1024);

    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.used_bytes(), 0);
    EXPECT_EQ(pool.capacity_bytes(), 1024);
}

TEST(ObjectPoolTest, CreateConstructsObject) {
    int constructions = 0;
    int destructions = 0;

    runtime::ObjectPool<TrackedObject> pool(1024);

    TrackedObject* object = pool.create(42, constructions, destructions);
    
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->value, 42);
    EXPECT_EQ(constructions, 1);
    EXPECT_EQ(destructions, 0);
    EXPECT_EQ(pool.size(),1);
    EXPECT_GT(pool.used_bytes(), 0);   

}

TEST(ObjectPoolTest, MultipleObjectsCanBeCreated) {
    int constructions = 0;
    int destructions = 0;

    runtime::ObjectPool<TrackedObject> pool(1024);

    TrackedObject* object1 = pool.create(42, constructions, destructions);
    TrackedObject* object2 = pool.create(55, constructions, destructions);

    ASSERT_NE(object1, nullptr);
    ASSERT_NE(object2, nullptr);
    EXPECT_EQ(object1->value, 42);
    EXPECT_EQ(object2->value, 55);
    EXPECT_EQ(constructions, 2);
    EXPECT_EQ(destructions, 0);
    EXPECT_EQ(pool.size(),2);
    EXPECT_NE(object1, object2);
}

TEST(ObjectPoolTest, ResetDestroysAllObjectsAndRestoresEmptyState) {
    int constructions = 0;
    int destructions = 0;

    runtime::ObjectPool<TrackedObject> pool(1024);

    pool.create(42, constructions, destructions);
    pool.create(55, constructions, destructions);
    pool.create(67, constructions, destructions);

    EXPECT_EQ(constructions, 3);
    EXPECT_EQ(destructions, 0);
    EXPECT_EQ(pool.size(), 3);
    EXPECT_GT(pool.used_bytes(), 0);

    pool.reset();

    EXPECT_EQ(destructions, 3);
    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.used_bytes(), 0);
    EXPECT_EQ(pool.capacity_bytes(), 1024);
    
    pool.reset();

    EXPECT_EQ(destructions, 3);
}

TEST(ObjectPoolTest, PoolDestructorDestroysLiveObjects) {
    int constructions = 0;
    int destructions = 0;

    {
        runtime::ObjectPool<TrackedObject> pool(1024);

        pool.create(42, constructions, destructions);
        pool.create(55, constructions, destructions);

        EXPECT_EQ(constructions, 2);
        EXPECT_EQ(destructions, 0);
    }

    EXPECT_EQ(destructions, 2);
}

TEST(ObjectPoolTest, ObjectsAreDestroyedInReverse) {
    std::vector<int> destro_order;

    runtime::ObjectPool<OrderedTrackedObject> pool(1024);

    pool.create(1, destro_order);
    pool.create(2, destro_order);
    pool.create(3, destro_order);

    pool.reset();

    EXPECT_EQ(destro_order, std::vector<int>({3,2,1}));
}

TEST(ObjectPoolTest, StorageCanBeReusedAfterReset) {
    int constructions = 0;
    int destructions = 0;

    runtime::ObjectPool<TrackedObject> pool(1024);
    

    TrackedObject* first = pool.create(42, constructions, destructions);

    EXPECT_EQ(constructions, 1);
    EXPECT_EQ(destructions, 0);

    pool.reset();


    EXPECT_EQ(destructions, 1);

    TrackedObject* second = pool.create(55, constructions, destructions);

    EXPECT_EQ(first, second);
    EXPECT_EQ(second->value, 55);
    EXPECT_EQ(constructions, 2);
    EXPECT_EQ(destructions, 1);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_GT(pool.used_bytes(), 0);
}

TEST(ObjectPoolTest, CreateThrowsWhenPoolIsExhausted) {
    int constructions = 0;
    int destructions = 0;

    constexpr std::size_t object_count = 2;
    
    runtime::ObjectPool<TrackedObject> pool(sizeof(TrackedObject) * object_count + alignof(TrackedObject) - 1);

    pool.create(42, constructions, destructions);
    pool.create(55, constructions, destructions);

    EXPECT_THROW(pool.create(67, constructions, destructions), std::bad_alloc);
}

TEST(ObjectPoolTest, ConstructorFailureRewindsArena) {
    runtime::ObjectPool<ThrowingObject> pool(1024);

    const std::size_t used_before = pool.used_bytes();
    const std::size_t size_before = pool.size();
    
    EXPECT_THROW(pool.create(1, true), std::runtime_error);    

    
    EXPECT_EQ(pool.used_bytes(), used_before);
    EXPECT_EQ(pool.size(), size_before);

    ThrowingObject* object = pool.create(2, false);

    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->value, 2);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_GT(pool.used_bytes(), 0);
}

TEST(ObjectPoolTest, ConstructorFailurePreservesExistingObjects) {
    runtime::ObjectPool<ThrowingObject> pool(1024);

    ThrowingObject* object = pool.create(1, false);

    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->value, 1);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_GT(pool.used_bytes(), 0);

    const std::size_t used_before = pool.used_bytes();

    EXPECT_THROW(pool.create(2, true), std::runtime_error);

    EXPECT_EQ(object->value, 1);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_EQ(pool.used_bytes(), used_before);

    ThrowingObject* next = pool.create(3, false);

    ASSERT_NE(next, nullptr);
    EXPECT_NE(next, object);
    EXPECT_EQ(next->value, 3);

    EXPECT_EQ(object->value, 1);
    EXPECT_EQ(pool.size(), 2);
    EXPECT_GT(pool.used_bytes(), used_before);
}