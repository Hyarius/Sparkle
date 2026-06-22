#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk
{
	template <typename TGpuObject>
	class CachedOpenGLObjectCollection
	{
	private:
		struct Entry
		{
			std::uint64_t contextId = 0;
			std::unique_ptr<TGpuObject> object;
		};

		std::vector<Entry> _entries;
		mutable std::uint64_t _prunedAtGeneration = 0;

		template <typename TFactory>
		std::unique_ptr<TGpuObject> _build(
			std::uint64_t p_version,
			std::uint64_t p_contentVersion,
			TFactory &&p_factory)
		{
			std::unique_ptr<TGpuObject> object = std::forward<TFactory>(p_factory)();
			object->_version = p_version;
			object->_contentVersion = p_contentVersion;
			return object;
		}

		Entry *_findEntry(std::uint64_t p_contextId) noexcept
		{
			for (Entry &entry : _entries)
			{
				if (entry.contextId == p_contextId)
				{
					return &entry;
				}
			}

			return nullptr;
		}

		[[nodiscard]] const Entry *_findEntry(std::uint64_t p_contextId) const noexcept
		{
			for (const Entry &entry : _entries)
			{
				if (entry.contextId == p_contextId)
				{
					return &entry;
				}
			}

			return nullptr;
		}

		[[nodiscard]] bool _hasExpectedVersions(
			const Entry *p_entry,
			std::uint64_t p_version,
			std::uint64_t p_contentVersion) const noexcept
		{
			return p_entry != nullptr && p_entry->object->version() == p_version &&
				   p_entry->object->contentVersion() == p_contentVersion;
		}

		[[nodiscard]] bool _isDeadForeignContext(const Entry &p_entry, std::uint64_t p_currentContextId) const
		{
			return p_entry.contextId != p_currentContextId &&
				   spk::OpenGL::isContextAlive(p_entry.contextId) == false;
		}

		void _removeEntryAt(std::size_t p_index)
		{
			if (p_index != _entries.size() - 1)
			{
				_entries[p_index] = std::move(_entries.back());
			}

			_entries.pop_back();
		}

		void _removeEntry(Entry *p_entry)
		{
			assert(p_entry != nullptr);

			if (p_entry != &_entries.back())
			{
				*p_entry = std::move(_entries.back());
			}

			_entries.pop_back();
		}

		[[nodiscard]] bool _pruneDeadForeignContexts(std::uint64_t p_currentContextId)
		{
			const std::uint64_t currentGeneration = spk::OpenGL::contextDeathGeneration();

			if (currentGeneration == _prunedAtGeneration)
			{
				return false;
			}

			for (std::size_t index = 0; index < _entries.size();)
			{
				if (_isDeadForeignContext(_entries[index], p_currentContextId) == true)
				{
					_removeEntryAt(index);
				}
				else
				{
					++index;
				}
			}

			_prunedAtGeneration = currentGeneration;
			return true;
		}

		template <typename TFactory>
		Entry &_appendEntry(
			std::uint64_t p_contextId,
			std::uint64_t p_version,
			std::uint64_t p_contentVersion,
			TFactory &&p_factory)
		{
			_entries.push_back(Entry{
				.contextId = p_contextId,
				.object = _build(p_version, p_contentVersion, std::forward<TFactory>(p_factory))});

			return _entries.back();
		}

		template <typename TFactory>
		Entry &_resolveEntry(
			Entry *p_entry,
			std::uint64_t p_contextId,
			std::uint64_t p_version,
			std::uint64_t p_contentVersion,
			TFactory &&p_factory)
		{
			if (p_entry != nullptr && p_entry->object->version() != p_version)
			{
				_removeEntry(p_entry);
				p_entry = nullptr;
			}

			if (p_entry == nullptr)
			{
				return _appendEntry(
					p_contextId,
					p_version,
					p_contentVersion,
					std::forward<TFactory>(p_factory));
			}

			return *p_entry;
		}

		template <typename TRefresh>
		void _refreshContentIfNeeded(
			Entry &p_entry,
			std::uint64_t p_contentVersion,
			TRefresh &&p_refresh)
		{
			if (p_entry.object->contentVersion() == p_contentVersion)
			{
				return;
			}

			std::forward<TRefresh>(p_refresh)(*p_entry.object);
			p_entry.object->_contentVersion = p_contentVersion;
		}

	public:
		CachedOpenGLObjectCollection() = default;

		~CachedOpenGLObjectCollection()
		{
			release();
		}

		CachedOpenGLObjectCollection(const CachedOpenGLObjectCollection &) = delete;
		CachedOpenGLObjectCollection &operator=(const CachedOpenGLObjectCollection &) = delete;

		CachedOpenGLObjectCollection(CachedOpenGLObjectCollection &&p_other) noexcept :
			_entries(std::move(p_other._entries)),
			_prunedAtGeneration(p_other._prunedAtGeneration)
		{
			p_other._entries.clear();
			p_other._prunedAtGeneration = 0;
		}

		CachedOpenGLObjectCollection &operator=(CachedOpenGLObjectCollection &&p_other) noexcept
		{
			if (this != &p_other)
			{
				release();
				_entries = std::move(p_other._entries);
				_prunedAtGeneration = p_other._prunedAtGeneration;
				p_other._entries.clear();
				p_other._prunedAtGeneration = 0;
			}

			return *this;
		}

		template <typename TFactory, typename TRefresh>
		TGpuObject &resolve(
			const spk::RenderContext &p_context,
			std::uint64_t p_version,
			std::uint64_t p_contentVersion,
			TFactory &&p_factory,
			TRefresh &&p_refresh)
		{
			assert(spk::OpenGL::isContextCurrent(p_context) == true);

			const std::uint64_t contextId = spk::OpenGL::contextIdOf(p_context);

			Entry *entry = _findEntry(contextId);

			if (_hasExpectedVersions(entry, p_version, p_contentVersion) == true)
			{
				return *entry->object;
			}

			if (_pruneDeadForeignContexts(contextId) == true)
			{
				entry = _findEntry(contextId);
			}

			Entry &resolvedEntry = _resolveEntry(
				entry,
				contextId,
				p_version,
				p_contentVersion,
				std::forward<TFactory>(p_factory));

			_refreshContentIfNeeded(
				resolvedEntry,
				p_contentVersion,
				std::forward<TRefresh>(p_refresh));

			return *resolvedEntry.object;
		}

		template <typename TFactory>
		TGpuObject &resolve(
			const spk::RenderContext &p_context,
			std::uint64_t p_version,
			TFactory &&p_factory)
		{
			return resolve(
				p_context,
				p_version,
				p_version,
				std::forward<TFactory>(p_factory),
				[](TGpuObject &) {
				});
		}

		[[nodiscard]] TGpuObject *find(const spk::RenderContext &p_context) const noexcept
		{
			const std::uint64_t contextId = spk::OpenGL::contextIdOf(p_context);
			const Entry *entry = _findEntry(contextId);

			if (entry == nullptr)
			{
				return nullptr;
			}

			return entry->object.get();
		}

		void release()
		{
			for (Entry &entry : _entries)
			{
				spk::OpenGL::releaseObject(std::move(entry.object));
			}

			_entries.clear();
			_prunedAtGeneration = 0;
		}
	};
}