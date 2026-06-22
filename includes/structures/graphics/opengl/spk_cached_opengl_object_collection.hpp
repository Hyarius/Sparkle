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

		// Real applications run 1-N contexts at once: a tiny vector with a linear
		// scan beats a hash map, per handle (and there are thousands of handles).
		std::vector<Entry> _entries;

		// Last generation seen during a prune pass. The prune loop (which calls
		// isContextAlive → registry mutex) is skipped when no context has died
		// since the last pass.
		mutable std::uint64_t _prunedAtGeneration = 0;

		template <typename TFactory>
		std::unique_ptr<TGpuObject> _build(std::uint64_t p_version, std::uint64_t p_contentVersion, TFactory &&p_factory)
		{
			std::unique_ptr<TGpuObject> object = p_factory();
			object->_version = p_version;
			object->_contentVersion = p_contentVersion;
			return object;
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

		// Two-tier resolution: a p_version mismatch destroys and rebuilds through
		// p_factory; a p_contentVersion mismatch alone updates in place through
		// p_refresh (e.g. glBufferSubData). p_context must be current.
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

			// Single-pass scan: if the entry for this context is clean, return
			// immediately (hot path — no mutex, no second scan).
			Entry *found = nullptr;
			for (Entry &entry : _entries)
			{
				if (entry.contextId == contextId)
				{
					if (entry.object->version() == p_version &&
						entry.object->contentVersion() == p_contentVersion)
					{
						return *entry.object;
					}
					found = &entry;
					break;
				}
			}

			// Prune entries whose context died with its GL objects. Gated behind a
			// relaxed atomic so the registry mutex is never acquired on the hot path
			// (contexts die ~never; the counter only changes on window close).
			const std::uint64_t currentGeneration = spk::OpenGL::contextDeathGeneration();
			if (currentGeneration != _prunedAtGeneration)
			{
				found = nullptr; // pointer may be invalidated by swap-and-pop below
				for (std::size_t index = 0; index < _entries.size();)
				{
					Entry &entry = _entries[index];
					if (entry.contextId != contextId && spk::OpenGL::isContextAlive(entry.contextId) == false)
					{
						entry = std::move(_entries.back());
						_entries.pop_back();
					}
					else
					{
						++index;
					}
				}
				_prunedAtGeneration = currentGeneration;

				// Re-find after prune since swap-and-pop may have reorganised the vector.
				for (Entry &entry : _entries)
				{
					if (entry.contextId == contextId)
					{
						found = &entry;
						break;
					}
				}
			}

			if (found != nullptr && found->object->version() != p_version)
			{
				// Structure changed: p_context is current, deleting in place is
				// legal. The entry is removed before rebuilding so a throwing
				// factory cannot leave a null object behind.
				if (found != &_entries.back())
				{
					*found = std::move(_entries.back());
				}
				_entries.pop_back();
				found = nullptr;
			}

			if (found == nullptr)
			{
				_entries.push_back(Entry{.contextId = contextId, .object = _build(p_version, p_contentVersion, p_factory)});
				found = &_entries.back();
			}
			else if (found->object->contentVersion() != p_contentVersion)
			{
				p_refresh(*found->object);
				found->object->_contentVersion = p_contentVersion;
			}

			return *found->object;
		}

		// Single-version resolution for create-once objects (programs, textures...).
		template <typename TFactory>
		TGpuObject &resolve(const spk::RenderContext &p_context, std::uint64_t p_version, TFactory &&p_factory)
		{
			return resolve(p_context, p_version, p_version, std::forward<TFactory>(p_factory), [](TGpuObject &) {
			});
		}

		[[nodiscard]] TGpuObject *find(const spk::RenderContext &p_context) const noexcept
		{
			const std::uint64_t contextId = spk::OpenGL::contextIdOf(p_context);
			for (const Entry &entry : _entries)
			{
				if (entry.contextId == contextId)
				{
					return entry.object.get();
				}
			}
			return nullptr;
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
