#include <gtest/gtest.h>

#include "structures/design_pattern/spk_singleton.hpp"

#include <stdexcept>
#include <string>

namespace
{
	class Counter : public spk::Singleton<Counter>
	{
		friend class spk::Singleton<Counter>;

	public:
		static inline int constructions = 0;
		static inline int destructions = 0;

		~Counter()
		{
			++destructions;
		}

		[[nodiscard]] const std::string &label() const noexcept
		{
			return _label;
		}

		void setLabel(std::string p_label)
		{
			_label = std::move(p_label);
		}

	private:
		explicit Counter(std::string p_label = "default") :
			_label(std::move(p_label))
		{
			++constructions;
		}

		std::string _label;
	};

	// Every case starts from no instance, whatever the previous one left behind.
	class SingletonTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			Counter::release();
			Counter::constructions = 0;
			Counter::destructions = 0;
		}

		void TearDown() override
		{
			Counter::release();
		}
	};
}

TEST_F(SingletonTest, HasNoInstanceUntilOneIsAskedFor)
{
	EXPECT_FALSE(Counter::isInstanciated());
	EXPECT_EQ(Counter::instance(), nullptr);
	EXPECT_EQ(Counter::constructions, 0) << "the instance is not created lazily on first use";

	EXPECT_THROW((void)Counter::get(), std::logic_error) << "a missing instanciation fails at the call that assumed it";
}

TEST_F(SingletonTest, InstanciateForwardsItsArgumentsAndPublishesTheInstance)
{
	Counter &counter = Counter::instanciate("configured");

	EXPECT_EQ(Counter::constructions, 1);
	EXPECT_TRUE(Counter::isInstanciated());
	EXPECT_EQ(&counter, Counter::instance());
	EXPECT_EQ(&counter, &Counter::get());
	EXPECT_EQ(Counter::get().label(), "configured");
}

TEST_F(SingletonTest, RefusesASecondInstanciation)
{
	(void)Counter::instanciate("first");

	// Two call sites both believing they configure the instance is a bug, not a race to win.
	EXPECT_THROW((void)Counter::instanciate("second"), std::logic_error);
	EXPECT_EQ(Counter::get().label(), "first");
	EXPECT_EQ(Counter::constructions, 1);
}

TEST_F(SingletonTest, ReleaseDestroysTheInstanceAndCanRunOnNothing)
{
	(void)Counter::instanciate();
	Counter::release();

	EXPECT_EQ(Counter::destructions, 1);
	EXPECT_FALSE(Counter::isInstanciated());

	EXPECT_NO_THROW(Counter::release()) << "releasing twice is not an error";
	EXPECT_EQ(Counter::destructions, 1);

	// And the type can be instanciated again afterwards.
	(void)Counter::instanciate("second life");
	EXPECT_EQ(Counter::get().label(), "second life");
	EXPECT_EQ(Counter::constructions, 2);
}

TEST_F(SingletonTest, AnInstanciatorOwnsTheInstanceForItsScope)
{
	{
		const spk::Singleton<Counter>::Instanciator counter("scoped");

		EXPECT_TRUE(Counter::isInstanciated());
		EXPECT_EQ(counter->label(), "scoped");
		EXPECT_EQ((*counter).label(), "scoped");

		counter->setLabel("changed");
		EXPECT_EQ(Counter::get().label(), "changed");
	}

	EXPECT_FALSE(Counter::isInstanciated()) << "the instance dies with the scope that owns it";
	EXPECT_EQ(Counter::destructions, 1);
}

TEST_F(SingletonTest, NestedInstanciatorsCountTheirOwnersInsteadOfTearingDownTheOuterOne)
{
	const spk::Singleton<Counter>::Instanciator outer("outer");
	{
		// A test that declares its own instanciator inside a program that already has one must
		// not release the program's instance when it returns.
		const spk::Singleton<Counter>::Instanciator inner;

		EXPECT_EQ(Counter::constructions, 1) << "the inner owner joins the existing instance";
		EXPECT_EQ(inner->label(), "outer") << "and does not reconfigure it";
	}

	EXPECT_TRUE(Counter::isInstanciated());
	EXPECT_EQ(Counter::destructions, 0);
	EXPECT_EQ(Counter::get().label(), "outer");
}
