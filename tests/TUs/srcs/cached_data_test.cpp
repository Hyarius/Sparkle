#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "spk_cached_data.hpp"

namespace
{
	struct CachedStruct
	{
		int value = 0;
		std::string name;

		CachedStruct() = default;

		CachedStruct(int p_value, std::string p_name) :
			value(p_value),
			name(std::move(p_name))
		{
		}
	};
}

TEST(CachedDataTest, DefaultConstructorStartsWithoutGeneratorAndWithoutCachedValue)
{
	spk::CachedData<int> cache;

	EXPECT_FALSE(cache.hasGenerator());
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, GetThrowsWhenGeneratorIsMissing)
{
	spk::CachedData<int> cache;

	EXPECT_THROW(cache.get(), std::logic_error);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, GeneratorIsLazy)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return 42;
	});

	EXPECT_EQ(generationCount, 0);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, GetGeneratesOnceAndCachesValue)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return 42;
	});

	int first = cache.get();
	int second = cache.get();

	EXPECT_EQ(first, 42);
	EXPECT_EQ(second, 42);
	EXPECT_EQ(generationCount, 1);
	EXPECT_TRUE(cache.isCached());
}

TEST(CachedDataTest, ConstGetGeneratesOnceAndCachesValue)
{
	int generationCount = 0;

	const spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return 99;
	});

	const int& first = cache.get();
	const int& second = cache.get();

	EXPECT_EQ(first, 99);
	EXPECT_EQ(second, 99);
	EXPECT_EQ(generationCount, 1);
	EXPECT_TRUE(cache.isCached());
}

TEST(CachedDataTest, DereferenceOperatorReturnsCachedValue)
{
	spk::CachedData<int> cache([]()
	{
		return 12;
	});

	EXPECT_EQ(*cache, 12);
	EXPECT_TRUE(cache.isCached());
}

TEST(CachedDataTest, ArrowOperatorProvidesMemberAccess)
{
	spk::CachedData<CachedStruct> cache([]()
	{
		return CachedStruct(5, "alpha");
	});

	EXPECT_EQ(cache->value, 5);
	EXPECT_EQ(cache->name, "alpha");
	EXPECT_TRUE(cache.isCached());
}

TEST(CachedDataTest, ConstArrowOperatorProvidesMemberAccess)
{
	const spk::CachedData<CachedStruct> cache([]()
	{
		return CachedStruct(7, "beta");
	});

	EXPECT_EQ(cache->value, 7);
	EXPECT_EQ(cache->name, "beta");
	EXPECT_TRUE(cache.isCached());
}

TEST(CachedDataTest, ReleaseClearsCachedValue)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return 10;
	});

	EXPECT_EQ(cache.get(), 10);
	EXPECT_TRUE(cache.isCached());
	EXPECT_EQ(generationCount, 1);

	cache.release();

	EXPECT_FALSE(cache.isCached());

	EXPECT_EQ(cache.get(), 10);
	EXPECT_EQ(generationCount, 2);
}

TEST(CachedDataTest, ReleaseDoesNothingWhenNothingIsCached)
{
	int destructorCount = 0;

	spk::CachedData<int> cache(
		[]()
		{
			return 1;
		},
		[&destructorCount](int&)
		{
			++destructorCount;
		});

	cache.release();

	EXPECT_EQ(destructorCount, 0);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, DestructorCallbackIsCalledOnRelease)
{
	int destructorCount = 0;
	int destroyedValue = -1;

	spk::CachedData<int> cache(
		[]()
		{
			return 123;
		},
		[&destructorCount, &destroyedValue](int& p_value)
		{
			++destructorCount;
			destroyedValue = p_value;
		});

	EXPECT_EQ(cache.get(), 123);
	cache.release();

	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, 123);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, DestructorCallbackIsCalledOnObjectDestructionWhenCached)
{
	int destructorCount = 0;
	int destroyedValue = -1;

	{
		spk::CachedData<int> cache(
			[]()
			{
				return 777;
			},
			[&destructorCount, &destroyedValue](int& p_value)
			{
				++destructorCount;
				destroyedValue = p_value;
			});

		EXPECT_EQ(cache.get(), 777);
		EXPECT_TRUE(cache.isCached());
	}

	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, 777);
}

TEST(CachedDataTest, ConfigureReplacesGeneratorAndClearsCachedValue)
{
	int firstGenerationCount = 0;
	int secondGenerationCount = 0;
	int destructorCount = 0;
	int destroyedValue = -1;

	spk::CachedData<int> cache(
		[&firstGenerationCount]()
		{
			++firstGenerationCount;
			return 10;
		},
		[&destructorCount, &destroyedValue](int& p_value)
		{
			++destructorCount;
			destroyedValue = p_value;
		});

	EXPECT_EQ(cache.get(), 10);
	EXPECT_EQ(firstGenerationCount, 1);
	EXPECT_TRUE(cache.isCached());

	cache.configure(
		[&secondGenerationCount]()
		{
			++secondGenerationCount;
			return 20;
		});

	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, 10);
	EXPECT_FALSE(cache.isCached());

	EXPECT_EQ(cache.get(), 20);
	EXPECT_EQ(secondGenerationCount, 1);
}

TEST(CachedDataTest, ConfigureWithoutCachedValueDoesNotCallDestructor)
{
	int destructorCount = 0;

	spk::CachedData<int> cache(
		[]()
		{
			return 10;
		},
		[&destructorCount](int&)
		{
			++destructorCount;
		});

	cache.configure([]()
	{
		return 20;
	});

	EXPECT_EQ(destructorCount, 0);
	EXPECT_FALSE(cache.isCached());
	EXPECT_EQ(cache.get(), 20);
}

TEST(CachedDataTest, SetByCopyStoresValueWithoutUsingGenerator)
{
	int generationCount = 0;

	spk::CachedData<std::string> cache([&generationCount]()
	{
		++generationCount;
		return std::string("generated");
	});

	std::string text = "manual";
	cache.set(text);

	EXPECT_TRUE(cache.isCached());
	EXPECT_EQ(cache.get(), "manual");
	EXPECT_EQ(generationCount, 0);
}

TEST(CachedDataTest, SetByMoveStoresValueWithoutUsingGenerator)
{
	int generationCount = 0;

	spk::CachedData<std::string> cache([&generationCount]()
	{
		++generationCount;
		return std::string("generated");
	});

	std::string text = "manual";
	cache.set(std::move(text));

	EXPECT_TRUE(cache.isCached());
	EXPECT_EQ(cache.get(), "manual");
	EXPECT_EQ(generationCount, 0);
}

TEST(CachedDataTest, SetReplacesCachedValueAndInvokesDestructorOnPreviousValue)
{
	int destructorCount = 0;
	std::string destroyedValue;

	spk::CachedData<std::string> cache(
		[]()
		{
			return std::string("generated");
		},
		[&destructorCount, &destroyedValue](std::string& p_value)
		{
			++destructorCount;
			destroyedValue = p_value;
		});

	cache.set(std::string("first"));
	cache.set(std::string("second"));

	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, "first");
	EXPECT_EQ(cache.get(), "second");
}

TEST(CachedDataTest, TakeReturnsNulloptWhenNoDataIsCached)
{
	spk::CachedData<int> cache([]()
	{
		return 1;
	});

	std::optional<int> result = cache.take();

	EXPECT_FALSE(result.has_value());
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, TakeMovesOutCachedValueWithoutCallingDestructor)
{
	int destructorCount = 0;

	spk::CachedData<std::string> cache(
		[]()
		{
			return std::string("generated");
		},
		[&destructorCount](std::string&)
		{
			++destructorCount;
		});

	cache.set(std::string("stored"));

	std::optional<std::string> result = cache.take();

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, "stored");
	EXPECT_FALSE(cache.isCached());
	EXPECT_EQ(destructorCount, 0);
}

TEST(CachedDataTest, RefreshRebuildsValue)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return generationCount * 10;
	});

	EXPECT_EQ(cache.get(), 10);
	EXPECT_EQ(generationCount, 1);

	EXPECT_EQ(cache.refresh(), 20);
	EXPECT_EQ(generationCount, 2);

	EXPECT_EQ(cache.get(), 20);
	EXPECT_EQ(generationCount, 2);
}

TEST(CachedDataTest, ConstRefreshRebuildsValue)
{
	int generationCount = 0;

	const spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return generationCount * 100;
	});

	EXPECT_EQ(cache.get(), 100);
	EXPECT_EQ(generationCount, 1);

	EXPECT_EQ(cache.refresh(), 200);
	EXPECT_EQ(generationCount, 2);

	EXPECT_EQ(cache.get(), 200);
	EXPECT_EQ(generationCount, 2);
}

TEST(CachedDataTest, HasGeneratorReflectsConfigurationState)
{
	spk::CachedData<int> cache;

	EXPECT_FALSE(cache.hasGenerator());

	cache.configure([]()
	{
		return 5;
	});

	EXPECT_TRUE(cache.hasGenerator());
}

TEST(CachedDataTest, ReleaseAfterSetCallsDestructor)
{
	int destructorCount = 0;
	int destroyedValue = -1;

	spk::CachedData<int> cache(
		[]()
		{
			return 0;
		},
		[&destructorCount, &destroyedValue](int& p_value)
		{
			++destructorCount;
			destroyedValue = p_value;
		});

	cache.set(321);
	cache.release();

	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, 321);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, SetThenRefreshDestroysManualValueAndRegenerates)
{
	int generationCount = 0;
	int destructorCount = 0;
	int destroyedValue = -1;

	spk::CachedData<int> cache(
		[&generationCount]()
		{
			++generationCount;
			return 1000 + generationCount;
		},
		[&destructorCount, &destroyedValue](int& p_value)
		{
			++destructorCount;
			destroyedValue = p_value;
		});

	cache.set(44);

	EXPECT_EQ(cache.refresh(), 1001);
	EXPECT_EQ(destructorCount, 1);
	EXPECT_EQ(destroyedValue, 44);
	EXPECT_EQ(generationCount, 1);
	EXPECT_EQ(cache.get(), 1001);
}

TEST(CachedDataTest, MoveOnlyValueCanBeStoredRetrievedAndTaken)
{
	spk::CachedData<std::unique_ptr<int>> cache([]()
	{
		return std::make_unique<int>(55);
	});

	cache.set(std::make_unique<int>(77));

	ASSERT_TRUE(cache.isCached());
	ASSERT_NE(cache.get(), nullptr);
	EXPECT_EQ(*cache.get(), 77);

	std::optional<std::unique_ptr<int>> extracted = cache.take();

	ASSERT_TRUE(extracted.has_value());
	ASSERT_NE(*extracted, nullptr);
	EXPECT_EQ(**extracted, 77);
	EXPECT_FALSE(cache.isCached());
}

TEST(CachedDataTest, GeneratorRunsAgainAfterTake)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return generationCount;
	});

	EXPECT_EQ(cache.get(), 1);
	EXPECT_EQ(generationCount, 1);

	std::optional<int> extracted = cache.take();
	ASSERT_TRUE(extracted.has_value());
	EXPECT_EQ(*extracted, 1);

	EXPECT_EQ(cache.get(), 2);
	EXPECT_EQ(generationCount, 2);
}

TEST(CachedDataTest, CachedReferenceRemainsSameObjectUntilReleased)
{
	spk::CachedData<std::string> cache([]()
	{
		return std::string("persistent");
	});

	const std::string* firstAddress = &(cache.get());
	const std::string* secondAddress = &(cache.get());

	EXPECT_EQ(firstAddress, secondAddress);
	EXPECT_EQ(*firstAddress, "persistent");
}

TEST(CachedDataTest, ManualSetKeepsCachedStateUntilReleased)
{
	int generationCount = 0;

	spk::CachedData<int> cache([&generationCount]()
	{
		++generationCount;
		return 10;
	});

	cache.set(88);

	EXPECT_TRUE(cache.isCached());
	EXPECT_EQ(cache.get(), 88);
	EXPECT_EQ(generationCount, 0);

	cache.release();

	EXPECT_FALSE(cache.isCached());
	EXPECT_EQ(cache.get(), 10);
	EXPECT_EQ(generationCount, 1);
}