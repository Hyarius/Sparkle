#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace spk
{
	// A single instance of TType with an explicitly owned lifetime.
	//
	// The instance is not created on first use and does not live until process exit: a scope
	// creates it (through Instanciator or instanciate()) and a scope destroys it. That keeps
	// construction order, construction failure and teardown order all observable - a lazily
	// created global would hide a load error behind whichever call site happened to run first.
	//
	// Instanciator is the intended entry point: declare one where the instance should live,
	// usually main(), and it is released when that scope ends. Nesting is counted, so a test
	// that declares its own Instanciator inside a program that already has one does not tear
	// down the outer instance.
	//
	//     spk::Singleton<Translator>::Instanciator translator;
	//     translator->load(path);
	//
	// TType keeps its constructors private and befriends its Singleton:
	//
	//     class Translator : public spk::Singleton<Translator>
	//     {
	//         friend class spk::Singleton<Translator>;
	//     private:
	//         Translator() = default;
	//     };
	//
	// The accessors are mutex-guarded, so instanciation and release are safe from any thread.
	// TType's own methods are not: guard those yourself when it is shared across threads.
	template <typename TType>
	class Singleton
	{
	public:
		using Type = TType;

		// Owns the instance for the length of its own scope, counting nested owners.
		class Instanciator
		{
		private:
			static inline std::size_t _owners = 0;

		public:
			template <typename... TArguments>
			explicit Instanciator(TArguments &&...p_arguments)
			{
				const std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				if (Singleton<TType>::instance() == nullptr)
				{
					Singleton<TType>::instanciate(std::forward<TArguments>(p_arguments)...);
				}
				++_owners;
			}

			~Instanciator()
			{
				const std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				--_owners;
				if (_owners == 0)
				{
					Singleton<TType>::release();
				}
			}

			Instanciator(const Instanciator &) = delete;
			Instanciator &operator=(const Instanciator &) = delete;
			Instanciator(Instanciator &&) = delete;
			Instanciator &operator=(Instanciator &&) = delete;

			[[nodiscard]] TType *operator->() const
			{
				return &Singleton<TType>::get();
			}

			[[nodiscard]] TType &operator*() const
			{
				return Singleton<TType>::get();
			}
		};

		// Throws std::logic_error rather than quietly returning the existing instance: two
		// call sites both believing they configure the instance is a bug, not a race to win.
		template <typename... TArguments>
		static TType &instanciate(TArguments &&...p_arguments)
		{
			const std::lock_guard<std::recursive_mutex> lock(mutex());

			if (_instance != nullptr)
			{
				throw std::logic_error("singleton is already instanciated");
			}

			_instance.reset(new TType(std::forward<TArguments>(p_arguments)...));
			return *_instance;
		}

		// nullptr when no instance exists. Prefer get() where absence is a bug.
		[[nodiscard]] static TType *instance() noexcept
		{
			const std::lock_guard<std::recursive_mutex> lock(mutex());
			return _instance.get();
		}

		// Throws std::logic_error when no instance exists, so a missing instanciation fails at
		// the call that assumed it rather than through a null dereference somewhere later.
		[[nodiscard]] static TType &get()
		{
			const std::lock_guard<std::recursive_mutex> lock(mutex());

			if (_instance == nullptr)
			{
				throw std::logic_error("singleton is not instanciated");
			}
			return *_instance;
		}

		[[nodiscard]] static bool isInstanciated() noexcept
		{
			const std::lock_guard<std::recursive_mutex> lock(mutex());
			return _instance != nullptr;
		}

		static void release() noexcept
		{
			const std::lock_guard<std::recursive_mutex> lock(mutex());
			_instance.reset();
		}

		[[nodiscard]] static std::recursive_mutex &mutex() noexcept
		{
			// Function-local so it is alive before any other static in any translation unit
			// can reach for it.
			static std::recursive_mutex result;
			return result;
		}

	protected:
		Singleton() = default;
		~Singleton() = default;

		Singleton(const Singleton &) = delete;
		Singleton &operator=(const Singleton &) = delete;
		Singleton(Singleton &&) = delete;
		Singleton &operator=(Singleton &&) = delete;

	private:
		static inline std::unique_ptr<TType> _instance = nullptr;
	};
}
