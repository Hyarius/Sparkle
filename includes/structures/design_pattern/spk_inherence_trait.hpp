#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType>
	class HierarchyTrait
	{
	private:
		// This non-owning hierarchy is single-threaded. Lifetime tokens only make
		// deferred mutations safe when nodes are destroyed during a traversal.
		struct LifetimeToken
		{
		};
		enum class DeferredMutationType
		{
			SetParent,
			ClearChildren
		};

		struct DeferredMutation
		{
			DeferredMutationType type;
			TType *node = nullptr;
			std::weak_ptr<LifetimeToken> nodeAliveToken;

			TType *parent = nullptr;
			std::weak_ptr<LifetimeToken> parentAliveToken;
		};

		struct TraversalState
		{
			HierarchyTrait<TType> *owner = nullptr;
			std::size_t nbBlocks = 0;
			bool requiresChildCompaction = false;
			std::deque<DeferredMutation> deferredMutations;
		};

		TType *_parent = nullptr;
		std::vector<TType *> _children;
		std::shared_ptr<TraversalState> _traversalState = std::make_shared<TraversalState>();
		std::shared_ptr<LifetimeToken> _aliveToken = std::make_shared<LifetimeToken>();

		[[nodiscard]] static HierarchyTrait<TType> *_trait(TType *p_node)
		{
			return static_cast<HierarchyTrait<TType> *>(p_node);
		}

		[[nodiscard]] static const HierarchyTrait<TType> *_trait(const TType *p_node)
		{
			return static_cast<const HierarchyTrait<TType> *>(p_node);
		}

		[[nodiscard]] bool _isTraversalBlocked() const
		{
			return (_traversalState != nullptr && _traversalState->nbBlocks != 0);
		}

		[[nodiscard]] static DeferredMutation _makeSetParentMutation(TType *p_node, TType *p_parent)
		{
			DeferredMutation result{
				.type = DeferredMutationType::SetParent,
				.node = p_node,
				.parent = p_parent};

			if (p_node != nullptr)
			{
				result.nodeAliveToken = _trait(p_node)->_aliveToken;
			}

			if (p_parent != nullptr)
			{
				result.parentAliveToken = _trait(p_parent)->_aliveToken;
			}

			return result;
		}

		[[nodiscard]] static DeferredMutation _makeClearChildrenMutation(TType *p_node)
		{
			DeferredMutation result{
				.type = DeferredMutationType::ClearChildren,
				.node = p_node};

			if (p_node != nullptr)
			{
				result.nodeAliveToken = _trait(p_node)->_aliveToken;
			}

			return result;
		}

		[[nodiscard]] static bool _isMutationTargetAlive(const DeferredMutation &p_mutation)
		{
			if (p_mutation.node == nullptr)
			{
				return false;
			}

			if (p_mutation.nodeAliveToken.expired() == true)
			{
				return false;
			}

			return true;
		}

		[[nodiscard]] static bool _isMutationParentAlive(const DeferredMutation &p_mutation)
		{
			if (p_mutation.parent == nullptr)
			{
				return true;
			}

			return (p_mutation.parentAliveToken.expired() == false);
		}

		[[nodiscard]] static bool _isMutationValid(const DeferredMutation &p_mutation)
		{
			return (_isMutationTargetAlive(p_mutation) == true && _isMutationParentAlive(p_mutation) == true);
		}

		[[nodiscard]] static HierarchyTrait<TType> *_blockedChildListOwner(TType *p_node)
		{
			if (p_node == nullptr)
			{
				return nullptr;
			}

			HierarchyTrait<TType> *trait = _trait(p_node);
			return (trait->_isTraversalBlocked() == true ? trait : nullptr);
		}

		void _deferMutation(DeferredMutation p_mutation)
		{
			if (_traversalState == nullptr)
			{
				return;
			}

			_traversalState->deferredMutations.push_back(std::move(p_mutation));
		}

		void _compactChildren() noexcept
		{
			_children.erase(std::remove(_children.begin(), _children.end(), nullptr), _children.end());
		}

		void _flushDeferredMutations()
		{
			if (_traversalState == nullptr)
			{
				return;
			}

			while (_traversalState->deferredMutations.empty() == false)
			{
				if (_traversalState->requiresChildCompaction)
				{
					_compactChildren();
					_traversalState->requiresChildCompaction = false;
				}
				std::deque<DeferredMutation> mutations = std::move(_traversalState->deferredMutations);
				_traversalState->deferredMutations.clear();

				for (const DeferredMutation &mutation : mutations)
				{
					if (_isMutationValid(mutation) == false)
					{
						continue;
					}

					switch (mutation.type)
					{
					case DeferredMutationType::SetParent:
						_trait(mutation.node)->setParent(mutation.parent);
						break;

					case DeferredMutationType::ClearChildren:
						_trait(mutation.node)->clearChildren();
						break;
					}
				}
			}
		}

		[[nodiscard]] bool _wouldCreateCycle(const TType *p_parent) const noexcept
		{
			for (const TType *current = p_parent; current != nullptr; current = current->parent())
			{
				if (current == static_cast<const TType *>(this))
				{
					return true;
				}
			}
			return false;
		}

		void _setParentImmediate(TType *p_parent)
		{
			TType *self = static_cast<TType *>(this);
			if (_parent == p_parent)
			{
				return;
			}
			TType *oldParent = _parent;
			if (p_parent != nullptr)
			{
				// Insert first: an allocation failure must leave the old relationship intact.
				p_parent->_children.emplace_back(self);
			}
			if (oldParent != nullptr)
			{
				auto oldIterator = std::find(oldParent->_children.begin(), oldParent->_children.end(), self);
				if (oldIterator != oldParent->_children.end())
				{
					oldParent->_children.erase(oldIterator);
				}
			}
			_parent = p_parent;
			if (oldParent != nullptr)
			{
				_trait(oldParent)->_onChildRemoved(self);
			}
			if (p_parent != nullptr)
			{
				_trait(p_parent)->_onChildAdded(self);
			}
			_onParentChanged(oldParent, _parent);
		}

		void _clearChildrenImmediate()
		{
			while (_children.empty() == false)
			{
				TType *child = _children.back();

				if (child == nullptr)
				{
					_children.pop_back();
					continue;
				}

				_trait(child)->_setParentImmediate(nullptr);
			}
		}

	public:
		class ChildrenTraversalGuard
		{
		private:
			std::weak_ptr<TraversalState> _state;

		public:
			explicit ChildrenTraversalGuard(const HierarchyTrait<TType> *p_owner)
			{
				if (p_owner == nullptr || p_owner->_traversalState == nullptr)
				{
					return;
				}

				_state = p_owner->_traversalState;
				if (p_owner->_traversalState->nbBlocks == std::numeric_limits<std::size_t>::max())
				{
					throw std::overflow_error("spk::HierarchyTrait: traversal guard overflow");
				}
				++p_owner->_traversalState->nbBlocks;
			}

			ChildrenTraversalGuard() = default;

			ChildrenTraversalGuard(const ChildrenTraversalGuard &) = delete;
			ChildrenTraversalGuard &operator=(const ChildrenTraversalGuard &) = delete;

			ChildrenTraversalGuard(ChildrenTraversalGuard &&p_other) noexcept :
				_state(std::move(p_other._state))
			{
				p_other._state.reset();
			}

			ChildrenTraversalGuard &operator=(ChildrenTraversalGuard &&p_other) noexcept
			{
				if (this != &p_other)
				{
					_release(false);

					_state = std::move(p_other._state);
					p_other._state.reset();
				}

				return *this;
			}

			~ChildrenTraversalGuard() noexcept
			{
				_release(false);
			}

			void _release(bool p_throwIfInvalid)
			{
				std::shared_ptr<TraversalState> state = _state.lock();
				if (state == nullptr)
				{
					if (p_throwIfInvalid)
					{
						throw std::logic_error("spk::HierarchyTrait: invalid traversal guard release");
					}
					return;
				}

				if (state->nbBlocks == 0)
				{
					_state.reset();
					if (p_throwIfInvalid)
					{
						throw std::logic_error("spk::HierarchyTrait: invalid traversal guard release");
					}
					return;
				}
				--state->nbBlocks;

				HierarchyTrait<TType> *owner = state->owner;
				if (state->nbBlocks == 0 && owner != nullptr)
				{
					if (state->requiresChildCompaction)
					{
						owner->_compactChildren();
						state->requiresChildCompaction = false;
					}
					try
					{
						owner->_flushDeferredMutations();
					} catch (...)
					{
						if (p_throwIfInvalid)
						{
							throw;
						}
						std::terminate();
					}
				}

				_state.reset();
			}

			void release()
			{
				_release(true);
			}

			[[nodiscard]] bool isValid() const
			{
				return (_state.expired() == false);
			}
		};

	protected:
		HierarchyTrait()
		{
			static_assert(std::is_base_of_v<HierarchyTrait<TType>, TType>, "TType must inherit from spk::HierarchyTrait<TType>");

			_traversalState->owner = this;
		}

		explicit HierarchyTrait(TType *p_parent) :
			HierarchyTrait()
		{
			setParent(p_parent);
		}

		~HierarchyTrait() noexcept
		{
			_aliveToken.reset();
			if (_traversalState != nullptr)
			{
				_traversalState->owner = nullptr;
				_traversalState->deferredMutations.clear();
			}
			if (_parent != nullptr)
			{
				HierarchyTrait<TType> *parentTrait = _trait(_parent);
				auto iterator = std::find(_parent->_children.begin(), _parent->_children.end(), static_cast<TType *>(this));
				if (iterator != _parent->_children.end())
				{
					if (parentTrait->_isTraversalBlocked())
					{
						*iterator = nullptr;
						parentTrait->_traversalState->requiresChildCompaction = true;
					}
					else
					{
						_parent->_children.erase(iterator);
					}
				}
				_parent = nullptr;
			}
			for (TType *child : _children)
			{
				if (child != nullptr)
				{
					_trait(child)->_parent = nullptr;
				}
			}
			_children.clear();
		}

		virtual void _onChildAdded(TType *p_child) noexcept
		{
		}
		virtual void _onChildRemoved(TType *p_child) noexcept
		{
		}
		virtual void _onParentChanged(TType *p_oldParent, TType *p_newParent) noexcept
		{
		}

	public:
		HierarchyTrait(const HierarchyTrait &) = delete;
		HierarchyTrait &operator=(const HierarchyTrait &) = delete;

		HierarchyTrait(HierarchyTrait &&) = delete;
		HierarchyTrait &operator=(HierarchyTrait &&) = delete;

		[[nodiscard("The returned guard must remain alive during traversal")]]
		ChildrenTraversalGuard guardChildrenTraversal() const
		{
			return ChildrenTraversalGuard(this);
		}

		[[nodiscard]] bool isRoot() const noexcept
		{
			return (_parent == nullptr);
		}

		[[nodiscard]] bool isLeaf() const noexcept
		{
			return (_children.empty() == true);
		}

		[[nodiscard]] bool isAncestorOf(const TType *p_node) const
		{
			const TType *current = p_node == static_cast<const TType *>(this) ? nullptr : p_node;

			while (current != nullptr)
			{
				if (current == static_cast<const TType *>(this))
				{
					return true;
				}

				current = current->parent();
			}

			return false;
		}

		[[nodiscard]] bool isDescendantOf(const TType *p_node) const
		{
			if (p_node == nullptr)
			{
				return false;
			}

			return p_node->isAncestorOf(static_cast<const TType *>(this));
		}

		void setParent(TType *p_parent)
		{
			if (p_parent == static_cast<TType *>(this) || _wouldCreateCycle(p_parent))
			{
				throw std::invalid_argument("spk::HierarchyTrait: parent would create a cycle");
			}
			HierarchyTrait<TType> *blockedOwner = _blockedChildListOwner(_parent);
			if (blockedOwner == nullptr)
			{
				blockedOwner = _blockedChildListOwner(p_parent);
			}

			if (blockedOwner != nullptr)
			{
				blockedOwner->_deferMutation(_makeSetParentMutation(static_cast<TType *>(this), p_parent));
				return;
			}

			_setParentImmediate(p_parent);
		}

		void addChild(TType *p_child)
		{
			if (p_child == nullptr)
			{
				throw std::invalid_argument("spk::HierarchyTrait: child cannot be null");
			}

			p_child->setParent(static_cast<TType *>(this));
		}

		void removeChild(TType *p_child)
		{
			if (p_child == nullptr)
			{
				throw std::invalid_argument("spk::HierarchyTrait: child cannot be null");
			}

			if (p_child->_parent != static_cast<TType *>(this))
			{
				return;
			}

			p_child->setParent(nullptr);
		}

		void clearChildren()
		{
			HierarchyTrait<TType> *blockedOwner = _blockedChildListOwner(static_cast<TType *>(this));
			if (blockedOwner != nullptr)
			{
				blockedOwner->_deferMutation(_makeClearChildrenMutation(static_cast<TType *>(this)));
				return;
			}

			_clearChildrenImmediate();
		}

		[[nodiscard]] TType *parent() noexcept
		{
			return _parent;
		}

		[[nodiscard]] const TType *parent() const noexcept
		{
			return _parent;
		}

		[[nodiscard]] bool hasParent() const noexcept
		{
			return (_parent != nullptr);
		}

		[[nodiscard]] bool hasChild(const TType *p_child) const noexcept
		{
			return (std::find(_children.begin(), _children.end(), p_child) != _children.end());
		}

		[[nodiscard]] std::size_t nbChildren() const noexcept
		{
			return _children.size();
		}

		[[nodiscard]] std::span<TType *const> children() noexcept
		{
			return _children;
		}

		[[nodiscard]] std::span<TType *const> children() const noexcept
		{
			return _children;
		}

		template <typename TCallback>
		void forEachChild(TCallback &&p_callback)
		{
			auto guard = guardChildrenTraversal();
			for (TType *child : _children)
			{
				if (child != nullptr)
				{
					p_callback(*child);
				}
			}
		}

		template <typename TCallback>
		void forEachChild(TCallback &&p_callback) const
		{
			auto guard = guardChildrenTraversal();
			for (TType *child : _children)
			{
				if (child != nullptr)
				{
					p_callback(*child);
				}
			}
		}
	};

	template <typename TType>
	using InherenceTrait = HierarchyTrait<TType>;
}
