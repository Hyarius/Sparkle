#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"

#include <algorithm>
#include <stdexcept>

namespace spk
{
	void RenderPipeline::_ensureMutable() const
	{
		if (_built)
		{
			throw std::logic_error("RenderPipeline has already been built");
		}
	}

	bool RenderPipeline::contains(std::string_view p_id) const noexcept
	{
		return !_built && _passes.contains(std::string(p_id));
	}

	spk::RenderPass &RenderPipeline::require(std::string_view p_id)
	{
		_ensureMutable();
		if (auto iterator = _passes.find(std::string(p_id)); iterator != _passes.end())
		{
			return *iterator->second;
		}
		throw std::invalid_argument("Missing render pass '" + std::string(p_id) + "'");
	}

	const spk::RenderPass &RenderPipeline::require(std::string_view p_id) const
	{
		return const_cast<spk::RenderPipeline *>(this)->require(p_id);
	}

	spk::RenderPass *RenderPipeline::find(std::string_view p_id) noexcept
	{
		if (_built)
		{
			return nullptr;
		}
		auto iterator = _passes.find(std::string(p_id));
		return iterator == _passes.end() ? nullptr : iterator->second.get();
	}

	const spk::RenderPass *RenderPipeline::find(std::string_view p_id) const noexcept
	{
		return const_cast<spk::RenderPipeline *>(this)->find(p_id);
	}

	std::size_t RenderPipeline::size() const noexcept
	{
		return _passes.size();
	}

	bool RenderPipeline::empty() const noexcept
	{
		return _passes.empty();
	}

	std::vector<spk::RenderPassDiagnostics> RenderPipeline::diagnostics(bool p_sorted) const
	{
		std::vector<const spk::RenderPass *> passes;
		passes.reserve(_orderedPasses.size());
		for (const spk::RenderPass *pass : _orderedPasses)
		{
			passes.push_back(pass);
		}
		if (p_sorted)
		{
			std::stable_sort(passes.begin(), passes.end(), [](const auto *p_lhs, const auto *p_rhs) {
				return p_lhs->priority() < p_rhs->priority();
			});
		}

		std::vector<spk::RenderPassDiagnostics> result;
		result.reserve(passes.size());
		for (const spk::RenderPass *pass : passes)
		{
			result.push_back(spk::makeRenderPassDiagnostics(*pass));
		}
		return result;
	}

	spk::RenderPlan RenderPipeline::build()
	{
		_ensureMutable();
		_built = true;
		for (spk::RenderPass *pass : _orderedPasses)
		{
			pass->_seal();
		}
		std::stable_sort(_orderedPasses.begin(), _orderedPasses.end(), [](const auto *p_lhs, const auto *p_rhs) {
			return p_lhs->priority() < p_rhs->priority();
		});

		std::vector<std::unique_ptr<spk::RenderPass>> ordered;
		ordered.reserve(_passes.size());
		for (const spk::RenderPass *pass : _orderedPasses)
		{
			ordered.push_back(std::move(_passes.at(pass->id())));
		}
		_passes.clear();
		return spk::RenderPlan(std::move(ordered));
	}
}
