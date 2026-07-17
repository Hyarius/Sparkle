#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/graphics/rendering/plan/spk_render_plan.hpp"

namespace spk
{
	class RenderPipeline
	{
	private:
		std::unordered_map<std::string, std::unique_ptr<spk::RenderPass>> _passes;
		std::vector<spk::RenderPass *> _orderedPasses;
		bool _built = false;

		void _ensureMutable() const;

	public:
		RenderPipeline() = default;
		RenderPipeline(const RenderPipeline &) = delete;
		RenderPipeline &operator=(const RenderPipeline &) = delete;
		RenderPipeline(RenderPipeline &&) noexcept = default;
		RenderPipeline &operator=(RenderPipeline &&) noexcept = default;

		template <typename TPass = spk::RenderPass, typename... TArguments>
			requires std::derived_from<TPass, spk::RenderPass>
		TPass &emplace(
			std::string p_id,
			spk::PriorizableTrait::Priority p_priority,
			spk::RenderPass::Description p_description,
			TArguments &&...p_arguments)
		{
			_ensureMutable();
			if (p_id.empty())
			{
				throw std::invalid_argument("Render pass ID cannot be empty");
			}
			if (_passes.contains(p_id))
			{
				throw std::invalid_argument("Duplicate render pass ID '" + p_id + "'");
			}

			auto pass = std::make_unique<TPass>(
			std::move(p_id),
			p_priority,
			std::move(p_description),
			std::forward<TArguments>(p_arguments)...);
			TPass &result = *pass;
			_orderedPasses.push_back(pass.get());
			_passes.emplace(result.id(), std::move(pass));
			return result;
		}

		[[nodiscard]] bool contains(std::string_view p_id) const noexcept;
		[[nodiscard]] spk::RenderPass &require(std::string_view p_id);
		[[nodiscard]] const spk::RenderPass &require(std::string_view p_id) const;
		[[nodiscard]] spk::RenderPass *find(std::string_view p_id) noexcept;
		[[nodiscard]] const spk::RenderPass *find(std::string_view p_id) const noexcept;

		template <typename TPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] TPass &require(std::string_view p_id)
		{
			spk::RenderPass &pass = require(p_id);
			if (auto *typed = dynamic_cast<TPass *>(&pass); typed != nullptr)
			{
				return *typed;
			}
			throw std::invalid_argument("Render pass '" + pass.id() + "' has a different concrete type");
		}

		template <typename TPass>
			requires std::derived_from<TPass, spk::RenderPass>
		[[nodiscard]] std::vector<std::reference_wrapper<TPass>> all()
		{
			std::vector<std::reference_wrapper<TPass>> result;
			if (_built)
			{
				return result;
			}
			for (spk::RenderPass *pass : _orderedPasses)
			{
				if (auto *typed = dynamic_cast<TPass *>(pass); typed != nullptr)
				{
					result.emplace_back(*typed);
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
