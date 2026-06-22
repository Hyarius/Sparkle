#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType>
	class HierarchyTrait
	{
	private:
		enum class DeferredMutationType
		{
			SetParent,
			ClearChildren
		};

		struct DeferredMutation
		{
			DeferredMutationType type;
			TType *node = nullptr;
			std::weak_ptr<void> nodeAliveToken;

			TType *parent = nullptr;
			std::weak_ptr<void> parentAliveToken;
			bool hasParentAliveToken = false;
		};

		struct TraversalState
		{
			HierarchyTrait<TType> *owner = nullptr;
			std::size_t nbBlocks = 0;
			std::vector<DeferredMutation> deferredMutations;
		};

		TType *_parent = nullptr;
		std::vector<TType *> _children;
		std::shared_ptr<TraversalState> _traversalState = std::make_shared<TraversalState>();
		std::shared_ptr<void> _aliveToken = std::make_shared<char>(0);

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
				result.hasParentAliveToken = true;
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

			if (p_mutation.hasParentAliveToken == false)
			{
				return false;
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

			if (p_mutation.type == DeferredMutationType::SetParent)
			{
				auto iterator = std::find_if(
					_traversalState->deferredMutations.begin(),
					_traversalState->deferredMutations.end(),
					[&](const DeferredMutation &p_existingMutation) {
						return p_existingMutation.type == DeferredMutationType::SetParent &&
							   p_existingMutation.node == p_mutation.node;
					});

				if (iterator != _traversalState->deferredMutations.end())
				{
					*iterator = std::move(p_mutation);
					return;
				}
			}

			_traversalState->deferredMutations.push_back(std::move(p_mutation));
		}

		void _flushDeferredMutations()
		{
			if (_traversalState == nullptr)
			{
				return;
			}

			while (_traversalState->deferredMutations.empty() == false)
			{
				std::vector<DeferredMutation> mutations = std::move(_traversalState->deferredMutations);
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

		void _setParentImmediate(TType *p_parent)
		{
			TType *self = static_cast<TType *>(this);

			if (p_parent == self)
			{
				return;
			}

			if (_parent == p_parent)
			{
				return;
			}

			if (p_parent != nullptr && self->isAncestorOf(p_parent) == true)
			{
				return;
			}

			TType *oldParent = _parent;

			if (_parent != nullptr)
			{
				auto oldIterator = std::find(_parent->_children.begin(), _parent->_children.end(), self);
				if (oldIterator != _parent->_children.end())
				{
					_parent->_children.erase(oldIterator);
					_trait(_parent)->_onChildRemoved(self);
				}
			}

			_parent = nullptr;

			if (p_parent != nullptr)
			{
				p_parent->_children.emplace_back(self);
				_parent = p_parent;
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
		class HierarchyMutationGuard
		{
		private:
			std::weak_ptr<TraversalState> _state;

		public:
			explicit HierarchyMutationGuard(const HierarchyTrait<TType> *p_owner)
			{
				if (p_owner == nullptr || p_owner->_traversalState == nullptr)
				{
					return;
				}

				_state = p_owner->_traversalState;
				++p_owner->_traversalState->nbBlocks;
			}

			HierarchyMutationGuard() = default;

			HierarchyMutationGuard(const HierarchyMutationGuard &) = delete;
			HierarchyMutationGuard &operator=(const HierarchyMutationGuard &) = delete;

			HierarchyMutationGuard(HierarchyMutationGuard &&p_other) noexcept :
				_state(std::move(p_other._state))
			{
				p_other._state.reset();
			}

			HierarchyMutationGuard &operator=(HierarchyMutationGuard &&p_other) noexcept
			{
				if (this != &p_other)
				{
					release();

					_state = std::move(p_other._state);
					p_other._state.reset();
				}

				return *this;
			}

			~HierarchyMutationGuard()
			{
				release();
			}

			void release()
			{
				std::shared_ptr<TraversalState> state = _state.lock();
				if (state == nullptr)
				{
					return;
				}

				if (state->nbBlocks != 0)
				{
					--state->nbBlocks;
				}

				HierarchyTrait<TType> *owner = state->owner;
				if (state->nbBlocks == 0 && owner != nullptr)
				{
					owner->_flushDeferredMutations();
				}

				_state.reset();
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

		~HierarchyTrait()
		{
			_aliveToken.reset();

			if (_traversalState != nullptr)
			{
				_traversalState->owner = nullptr;
				_traversalState->deferredMutations.clear();
			}

			_setParentImmediate(nullptr);
			_clearChildrenImmediate();

			if (_traversalState != nullptr)
			{
				_traversalState->owner = nullptr;
			}
		}

		virtual void _onChildAdded(TType *p_child)
		{
		}
		virtual void _onChildRemoved(TType *p_child)
		{
		}
		virtual void _onParentChanged(TType *p_oldParent, TType *p_newParent)
		{
		}

	public:
		HierarchyTrait(const HierarchyTrait &) = delete;
		HierarchyTrait &operator=(const HierarchyTrait &) = delete;

		HierarchyTrait(HierarchyTrait &&) noexcept = delete;
		HierarchyTrait &operator=(HierarchyTrait &&) noexcept = delete;

		[[nodiscard]] bool isRoot() const
		{
			return (_parent == nullptr);
		}

		[[nodiscard]] bool isLeaf() const
		{
			return (_children.empty() == true);
		}

		[[nodiscard]] bool isAncestorOf(const TType *p_node) const
		{
			const TType *current = p_node;

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
				return;
			}

			p_child->setParent(static_cast<TType *>(this));
		}

		void removeChild(TType *p_child)
		{
			if (p_child == nullptr)
			{
				return;
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

		[[nodiscard]] TType *parent() const
		{
			return _parent;
		}

		[[nodiscard]] bool hasParent() const
		{
			return (_parent != nullptr);
		}

		[[nodiscard]] bool hasChild(const TType *p_child) const
		{
			return (std::find(_children.begin(), _children.end(), p_child) != _children.end());
		}

		[[nodiscard]] std::size_t nbChildren() const
		{
			return _children.size();
		}

		[[nodiscard]] const std::vector<TType *> &children() const
		{
			return _children;
		}
	};

	template <typename TType>
	using InherenceTrait = HierarchyTrait<TType>;
}