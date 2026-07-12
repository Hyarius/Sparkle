#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/graphics/rendering/plan/spk_render_plan.hpp"

namespace spk
{
	class RenderPassBucketPack
	{
	private:
		std::vector<std::unique_ptr<spk::RenderPass>> _passes;
		std::unordered_map<spk::RenderPass::Key, spk::RenderPass *> _index;
		std::unordered_map<std::uint64_t, std::string> _canonicalNames;
		std::size_t _nextDeclarationOrder = 0;
		bool _built = false;

		void _ensureMutable() const;

	public:
		RenderPassBucketPack() = default;
		RenderPassBucketPack(const RenderPassBucketPack &) = delete;
		RenderPassBucketPack &operator=(const RenderPassBucketPack &) = delete;
		RenderPassBucketPack(RenderPassBucketPack &&) noexcept = default;
		RenderPassBucketPack &operator=(RenderPassBucketPack &&) noexcept = default;

		void registerPassType(spk::RenderPass::TypeId p_type, std::string p_canonicalName);

		template <typename TPass = spk::RenderPass, typename... TArguments>
			requires std::derived_from<TPass, spk::RenderPass>
		TPass &emplacePass(
			const spk::RenderPass::Key &p_key,
			std::int32_t p_priority,
			std::string p_debugName,
			spk::RenderPass::Description p_description,
			TArguments &&...p_arguments)
		{
			_ensureMutable();
			if (_index.contains(p_key))
			{
				throw std::invalid_argument(
					"Duplicate render pass key type=" + std::to_string(p_key.type.value) +
					", scope=" + std::to_string(p_key.scope.value) +
					", instance=" + std::to_string(p_key.instance) +
					" while declaring '" + p_debugName + "'");
			}

			auto pass = std::make_unique<TPass>(
				p_key,
				p_priority,
				_nextDeclarationOrder++,
				std::move(p_debugName),
				std::move(p_description),
				std::forward<TArguments>(p_arguments)...);
			TPass &result = *pass;
			_index.emplace(p_key, pass.get());
			_passes.push_back(std::move(pass));
			return result;
		}

		[[nodiscard]] bool contains(const spk::RenderPass::Key &p_key) const noexcept;
		[[nodiscard]] spk::RenderPass &require(const spk::RenderPass::Key &p_key);
		[[nodiscard]] const spk::RenderPass &require(const spk::RenderPass::Key &p_key) const;
		[[nodiscard]] spk::RenderPass *find(const spk::RenderPass::Key &p_key) noexcept;
		[[nodiscard]] const spk::RenderPass *find(const spk::RenderPass::Key &p_key) const noexcept;

		template <typename TPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] TPass &require(const spk::RenderPass::Key &p_key)
		{
			spk::RenderPass &pass = require(p_key);
			if (auto *typed = dynamic_cast<TPass *>(&pass); typed != nullptr)
			{
				return *typed;
			}
			throw std::invalid_argument("Render pass '" + pass.debugName() + "' has concrete type '" + typeid(pass).name() + "', not requested type '" + typeid(TPass).name() + "'");
		}

		template <typename TPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] const TPass &require(const spk::RenderPass::Key &p_key) const
		{
			return const_cast<spk::RenderPassBucketPack *>(this)->require<TPass>(p_key);
		}

		template <typename TPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] TPass *find(const spk::RenderPass::Key &p_key)
		{
			spk::RenderPass *pass = find(p_key);
			if (pass == nullptr)
			{
				return nullptr;
			}
			if (auto *typed = dynamic_cast<TPass *>(pass); typed != nullptr)
			{
				return typed;
			}
			throw std::invalid_argument("Render pass '" + pass->debugName() + "' has a different concrete type");
		}

		template <typename TPass = spk::RenderPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] std::vector<std::reference_wrapper<TPass>> passesOfType(
			spk::RenderPass::TypeId p_type,
			std::optional<spk::RenderPass::ScopeId> p_scope = std::nullopt)
		{
			std::vector<std::reference_wrapper<TPass>> result;
			for (const auto &pass : _passes)
			{
				if (pass->key().type != p_type || (p_scope.has_value() && pass->key().scope != *p_scope))
				{
					continue;
				}
				if (auto *typed = dynamic_cast<TPass *>(pass.get()); typed != nullptr)
				{
					result.emplace_back(*typed);
				}
				else
				{
					throw std::invalid_argument("Render pass '" + pass->debugName() + "' has a different concrete type");
				}
			}
			return result;
		}

		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] std::vector<spk::RenderPassDiagnostics> diagnostics(bool p_sorted = false) const;
		[[nodiscard]] spk::RenderPlan build();
	};
}
