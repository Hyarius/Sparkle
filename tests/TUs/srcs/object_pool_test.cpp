#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "spk_object_pool.hpp"

namespace
{
	struct TrackedObject
	{
		int value = 0;
		std::string text;

		inline static int constructorCount = 0;
		inline static int destructorCount = 0;

		TrackedObject()
		{
			++constructorCount;
		}

		explicit TrackedObject(int p_value) :
			value(p_value)
		{
			++constructorCount;
		}

		TrackedObject(int p_value, std::string p_text) :
			value(p_value),
			text(std::move(p_text))
		{
			++constructorCount;
		}

		~TrackedObject()
		{
			++destructorCount;
		}

		static void resetCounters()
		{
			constructorCount = 0;
			destructorCount = 0;
		}
	};

	struct ObjectPoolTest : public ::testing::Test
	{
		void SetUp() override
		{
			TrackedObject::resetCounters();
		}

		void TearDown() override
		{
			TrackedObject::resetCounters();
		}
	};
}

TEST_F(ObjectPoolTest, defaultConstructedPool_ShouldBeEmptyAndUnconfigured)
{
	spk::ObjectPool<TrackedObject> pool;

	EXPECT_TRUE(pool.empty());
	EXPECT_EQ(pool.size(), 0u);
	EXPECT_FALSE(pool.isClosed());
	EXPECT_FALSE(pool.hasGenerator());
	EXPECT_FALSE(pool.hasCleaner());
}

TEST_F(ObjectPoolTest, configure_ShouldRegisterGeneratorAndCleaner)
{
	spk::ObjectPool<TrackedObject> pool;

	pool.configure(
		[]()
		{
			return std::make_unique<TrackedObject>(42, "generated");
		},
		[](TrackedObject& p_object)
		{
			p_object.value = 0;
			p_object.text.clear();
		});

	EXPECT_TRUE(pool.hasGenerator());
	EXPECT_TRUE(pool.hasCleaner());
	EXPECT_FALSE(pool.isClosed());
}

TEST_F(ObjectPoolTest, obtain_WithoutGenerator_ShouldThrowLogicError)
{
	spk::ObjectPool<TrackedObject> pool;

	EXPECT_THROW(
		{
			auto object = pool.obtain();
			(void)object;
		},
		std::logic_error);
}

TEST_F(ObjectPoolTest, obtain_WithGenerator_ShouldCreateObjectWhenPoolIsEmpty)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(12, "fresh");
		});

	auto object = pool.obtain();

	ASSERT_NE(object, nullptr);
	EXPECT_EQ(object->value, 12);
	EXPECT_EQ(object->text, "fresh");
	EXPECT_EQ(pool.size(), 0u);
	EXPECT_EQ(TrackedObject::constructorCount, 1);
}

TEST_F(ObjectPoolTest, releasedHandle_ShouldReturnObjectToPool)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(7);
		});

	{
		auto object = pool.obtain();
		ASSERT_NE(object, nullptr);
		EXPECT_EQ(pool.size(), 0u);
	}

	EXPECT_EQ(pool.size(), 1u);
	EXPECT_FALSE(pool.empty());
}

TEST_F(ObjectPoolTest, obtain_ShouldReuseReturnedObject)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(7);
		});

	TrackedObject* firstAddress = nullptr;
	TrackedObject* secondAddress = nullptr;

	{
		auto firstObject = pool.obtain();
		firstAddress = firstObject.get();
	}

	{
		auto secondObject = pool.obtain();
		secondAddress = secondObject.get();
	}

	EXPECT_EQ(firstAddress, secondAddress);
	EXPECT_EQ(TrackedObject::constructorCount, 1);
}

TEST_F(ObjectPoolTest, cleaner_ShouldBeCalledWhenObtainingReusedObject)
{
	int cleanerCallCount = 0;

	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(5, "generated");
		},
		[&cleanerCallCount](TrackedObject& p_object)
		{
			++cleanerCallCount;
			p_object.value = 0;
			p_object.text = "clean";
		});

	{
		auto object = pool.obtain();
		EXPECT_EQ(object->value, 0);
		EXPECT_EQ(object->text, "clean");

		object->value = 99;
		object->text = "dirty";
	}

	{
		auto object = pool.obtain();
		EXPECT_EQ(object->value, 0);
		EXPECT_EQ(object->text, "clean");
	}

	EXPECT_EQ(cleanerCallCount, 2);
}

TEST_F(ObjectPoolTest, reserve_ShouldPreallocateObjectsUsingGenerator)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(10);
		});

	pool.reserve(3);

	EXPECT_EQ(pool.size(), 3u);
	EXPECT_EQ(TrackedObject::constructorCount, 3);
}

TEST_F(ObjectPoolTest, reserve_WithoutGenerator_ShouldThrowLogicError)
{
	spk::ObjectPool<TrackedObject> pool;

	EXPECT_THROW(pool.reserve(2), std::logic_error);
}

TEST_F(ObjectPoolTest, reserve_ShouldNotShrinkExistingPool)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(10);
		});

	pool.reserve(4);
	pool.reserve(2);

	EXPECT_EQ(pool.size(), 4u);
	EXPECT_EQ(TrackedObject::constructorCount, 4);
}

TEST_F(ObjectPoolTest, setMaximumCachedObjectCount_ShouldTrimStoredObjects)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		});

	pool.reserve(5);
	EXPECT_EQ(pool.size(), 5u);

	pool.setMaximumCachedObjectCount(2);

	EXPECT_EQ(pool.size(), 2u);
}

TEST_F(ObjectPoolTest, setMaximumCachedObjectCount_ShouldLimitFutureReturns)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		});

	pool.setMaximumCachedObjectCount(1);

	{
		auto firstObject = pool.obtain();
		auto secondObject = pool.obtain();

		EXPECT_EQ(pool.size(), 0u);
	}

	EXPECT_EQ(pool.size(), 1u);
}

TEST_F(ObjectPoolTest, release_ShouldClearAvailableObjects)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(3);
		});

	pool.reserve(4);
	ASSERT_EQ(pool.size(), 4u);

	pool.release();

	EXPECT_EQ(pool.size(), 0u);
	EXPECT_TRUE(pool.empty());
}

TEST_F(ObjectPoolTest, release_ShouldInvokeRealDestructorOnStoredObjects)
{
	int realDestructorCallCount = 0;

	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(3);
		},
		nullptr,
		[&realDestructorCallCount](TrackedObject&)
		{
			++realDestructorCallCount;
		});

	pool.reserve(4);
	pool.release();

	EXPECT_EQ(realDestructorCallCount, 4);
	EXPECT_EQ(pool.size(), 0u);
}

TEST_F(ObjectPoolTest, configure_ShouldReleaseExistingStoredObjectsBeforeReplacingCallbacks)
{
	int firstDestructorCallCount = 0;

	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		},
		nullptr,
		[&firstDestructorCallCount](TrackedObject&)
		{
			++firstDestructorCallCount;
		});

	pool.reserve(3);
	ASSERT_EQ(pool.size(), 3u);

	pool.configure(
		[]()
		{
			return std::make_unique<TrackedObject>(9);
		});

	EXPECT_EQ(firstDestructorCallCount, 3);
	EXPECT_EQ(pool.size(), 0u);
	EXPECT_TRUE(pool.hasGenerator());
	EXPECT_FALSE(pool.hasCleaner());
}

TEST_F(ObjectPoolTest, obtain_ShouldThrowIfGeneratorReturnsNullptr)
{
	spk::ObjectPool<TrackedObject> pool(
		[]() -> std::unique_ptr<TrackedObject>
		{
			return nullptr;
		});

	EXPECT_THROW(
		{
			auto object = pool.obtain();
			(void)object;
		},
		std::logic_error);
}

TEST_F(ObjectPoolTest, reserve_ShouldThrowIfGeneratorReturnsNullptr)
{
	spk::ObjectPool<TrackedObject> pool(
		[]() -> std::unique_ptr<TrackedObject>
		{
			return nullptr;
		});

	EXPECT_THROW(pool.reserve(1), std::logic_error);
}

TEST_F(ObjectPoolTest, close_ShouldMarkPoolClosedAndClearStoredObjects)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(4);
		});

	pool.reserve(2);
	ASSERT_EQ(pool.size(), 2u);

	pool.close();

	EXPECT_TRUE(pool.isClosed());
	EXPECT_EQ(pool.size(), 0u);
}

TEST_F(ObjectPoolTest, closedPool_ShouldRejectConfigure)
{
	spk::ObjectPool<TrackedObject> pool;
	pool.close();

	EXPECT_THROW(
		pool.configure(
			[]()
			{
				return std::make_unique<TrackedObject>(1);
			}),
		std::runtime_error);
}

TEST_F(ObjectPoolTest, closedPool_ShouldRejectSetMaximumCachedObjectCount)
{
	spk::ObjectPool<TrackedObject> pool;
	pool.close();

	EXPECT_THROW(pool.setMaximumCachedObjectCount(2), std::runtime_error);
}

TEST_F(ObjectPoolTest, closedPool_ShouldRejectReserve)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		});

	pool.close();

	EXPECT_THROW(pool.reserve(2), std::runtime_error);
}

TEST_F(ObjectPoolTest, closedPool_ShouldRejectObtain)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		});

	pool.close();

	EXPECT_THROW(
		{
			auto object = pool.obtain();
			(void)object;
		},
		std::runtime_error);
}

TEST_F(ObjectPoolTest, closedPool_ShouldRejectRelease)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(1);
		});

	pool.close();

	EXPECT_THROW(pool.release(), std::runtime_error);
}

TEST_F(ObjectPoolTest, returnedHandleAfterPoolClose_ShouldDeleteObjectInsteadOfRecyclingIt)
{
	int realDestructorCallCount = 0;

	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(8);
		},
		nullptr,
		[&realDestructorCallCount](TrackedObject&)
		{
			++realDestructorCallCount;
		});

	auto object = pool.obtain();
	TrackedObject* objectAddress = object.get();

	pool.close();

	object.reset();

	EXPECT_TRUE(pool.isClosed());
	EXPECT_EQ(pool.size(), 0u);
	EXPECT_EQ(realDestructorCallCount, 0);
	(void)objectAddress;
}

TEST_F(ObjectPoolTest, returnedHandle_WhenPoolIsFull_ShouldDeleteObjectInsteadOfCachingIt)
{
	int realDestructorCallCount = 0;

	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(8);
		},
		nullptr,
		[&realDestructorCallCount](TrackedObject&)
		{
			++realDestructorCallCount;
		});

	pool.setMaximumCachedObjectCount(0);

	{
		auto object = pool.obtain();
		ASSERT_NE(object, nullptr);
	}

	EXPECT_EQ(pool.size(), 0u);
	EXPECT_EQ(realDestructorCallCount, 1);
}

TEST_F(ObjectPoolTest, objectPoolConstructor_ShouldConfigureCallbacksImmediately)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(77, "configured");
		},
		[](TrackedObject& p_object)
		{
			p_object.text += "_clean";
		});

	EXPECT_TRUE(pool.hasGenerator());
	EXPECT_TRUE(pool.hasCleaner());

	auto object = pool.obtain();

	EXPECT_EQ(object->value, 77);
	EXPECT_EQ(object->text, "configured_clean");
}

TEST_F(ObjectPoolTest, reusedObject_ShouldBeSameAllocationButCleanedBetweenUsages)
{
	spk::ObjectPool<TrackedObject> pool(
		[]()
		{
			return std::make_unique<TrackedObject>(100, "initial");
		},
		[](TrackedObject& p_object)
		{
			p_object.value = -1;
			p_object.text = "reset";
		});

	TrackedObject* firstAddress = nullptr;
	TrackedObject* secondAddress = nullptr;

	{
		auto firstObject = pool.obtain();
		firstAddress = firstObject.get();
		firstObject->value = 555;
		firstObject->text = "used";
	}

	{
		auto secondObject = pool.obtain();
		secondAddress = secondObject.get();

		EXPECT_EQ(secondObject->value, -1);
		EXPECT_EQ(secondObject->text, "reset");
	}

	EXPECT_EQ(firstAddress, secondAddress);
	EXPECT_EQ(TrackedObject::constructorCount, 1);
}

TEST_F(ObjectPoolTest, destroyingPool_ShouldInvalidateRecyclingButKeepOutstandingHandlesSafe)
{
	TrackedObject* savedAddress = nullptr;
	std::unique_ptr<TrackedObject, spk::ObjectPool<TrackedObject>::ReturnToPoolDeleter> savedHandle;

	{
		spk::ObjectPool<TrackedObject> pool(
			[]()
			{
				return std::make_unique<TrackedObject>(123);
			});

		auto object = pool.obtain();
		savedAddress = object.get();
		savedHandle = std::move(object);

		EXPECT_NE(savedAddress, nullptr);
	}

	EXPECT_NE(savedHandle.get(), nullptr);

	savedHandle.reset();
}