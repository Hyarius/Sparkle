#pragma once

#include <algorithm>
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

	public:
		InherenceTrait(const InherenceTrait&) = delete;
		InherenceTrait& operator=(const InherenceTrait&) = delete;

		InherenceTrait(InherenceTrait&&) noexcept = delete;
		InherenceTrait& operator=(InherenceTrait&&) noexcept = delete;

		virtual void addChild(TType* p_child)
		{
			if (p_child == nullptr || p_child == static_cast<TType*>(this))
			{
				return;
			}

			if (hasChild(p_child) == true)
			{
				return;
			}

			if (p_child->_parent != nullptr)
			{
				p_child->_parent->removeChild(p_child);
			}

			p_child->_parent = static_cast<TType*>(this);
			_children.emplace_back(p_child);
		}

		virtual void removeChild(TType* p_child)
		{
			if (p_child == nullptr)
			{
				return;
			}

			auto iterator = std::find(_children.begin(), _children.end(), p_child);
			if (iterator == _children.end())
			{
				return;
			}

			_children.erase(iterator);
			p_child->_parent = nullptr;
		}

		void clearChildren()
		{
			for (TType* child : _children)
			{
				if (child != nullptr)
				{
					child->_parent = nullptr;
				}
			}

			_children.clear();
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

		size_t nbChildren() const
		{
			return _children.size();
		}

		virtual std::vector<TType*>& children()
		{
			return _children;
		}

		virtual const std::vector<TType*>& children() const
		{
			return _children;
		}
	};
}