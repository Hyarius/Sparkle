#pragma once

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace spk
{
	template <typename TType>
	class InherenceTrait
	{
	private:
		TType* _parent = nullptr;
		std::vector<TType*> _children;

	protected:
		InherenceTrait()
		{
			static_assert(std::is_base_of_v<InherenceTrait<TType>, TType>, "TType must inherit from spk::InherenceTrait<TType>");
		}

		~InherenceTrait()
		{
			if (_parent != nullptr)
			{
				_parent->removeChild(static_cast<TType*>(this));
			}

			clearChildren();
		}

		virtual void onChildAdded(TType* p_child) {}
		virtual void onChildRemoved(TType* p_child) {}
		virtual void onParentChanged(TType* p_oldParent, TType* p_newParent) {}

	public:
		InherenceTrait(const InherenceTrait&) = delete;
		InherenceTrait& operator=(const InherenceTrait&) = delete;

		InherenceTrait(InherenceTrait&&) noexcept = delete;
		InherenceTrait& operator=(InherenceTrait&&) noexcept = delete;

		bool isRoot() const
		{
			return (_parent == nullptr);
		}

		bool isLeaf() const
		{
			return (_children.empty() == true);
		}

		bool isAncestorOf(const TType* p_node) const
		{
			const TType* current = p_node;
			while (current != nullptr)
			{
				if (current == static_cast<const TType*>(this))
				{
					return true;
				}

				current = current->parent();
			}

			return false;
		}

		bool isDescendantOf(const TType* p_node) const
		{
			if (p_node == nullptr)
			{
				return false;
			}

			return p_node->isAncestorOf(static_cast<const TType*>(this));
		}

		void setParent(TType* p_parent)
		{
			TType* self = static_cast<TType*>(this);

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

			TType* oldParent = _parent;

			if (_parent != nullptr)
			{
				auto oldIterator = std::find(_parent->_children.begin(), _parent->_children.end(), self);
				if (oldIterator != _parent->_children.end())
				{
					_parent->_children.erase(oldIterator);
					static_cast<InherenceTrait<TType>*>(_parent)->onChildRemoved(self);
				}
			}

			_parent = nullptr;

			if (p_parent != nullptr)
			{
				p_parent->_children.emplace_back(self);
				_parent = p_parent;
				static_cast<InherenceTrait<TType>*>(p_parent)->onChildAdded(self);
			}

			onParentChanged(oldParent, _parent);
		}

		void addChild(TType* p_child)
		{
			if (p_child == nullptr)
			{
				return;
			}

			p_child->setParent(static_cast<TType*>(this));
		}

		void removeChild(TType* p_child)
		{
			if (p_child == nullptr)
			{
				return;
			}

			if (p_child->_parent != static_cast<TType*>(this))
			{
				return;
			}

			p_child->setParent(nullptr);
		}

		void clearChildren()
		{
			while (_children.empty() == false)
			{
				TType* child = _children.back();
				if (child == nullptr)
				{
					_children.pop_back();
					continue;
				}

				child->setParent(nullptr);
			}
		}

		TType* parent() const
		{
			return _parent;
		}

		bool hasParent() const
		{
			return (_parent != nullptr);
		}

		bool hasChild(const TType* p_child) const
		{
			return (std::find(_children.begin(), _children.end(), p_child) != _children.end());
		}

		std::size_t nbChildren() const
		{
			return _children.size();
		}

		const std::vector<TType*>& children() const
		{
			return _children;
		}
	};
}