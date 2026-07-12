#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	void RenderPassBucketPack::_ensureMutable() const
	{
		if (_built)
		{
			throw std::logic_error("RenderPassBucketPack has already been built");
		}
	}

	void RenderPassBucketPack::registerPassType(spk::RenderPass::TypeId p_type, std::string p_canonicalName)
	{
		_ensureMutable();
		if (p_canonicalName.empty())
		{
			throw std::invalid_argument("Render pass canonical name cannot be empty");
		}
		if (auto iterator = _canonicalNames.find(p_type.value); iterator != _canonicalNames.end())
		{
			if (iterator->second != p_canonicalName)
			{
				throw std::invalid_argument(
					"Render pass type hash collision between '" + iterator->second + "' and '" + p_canonicalName + "'");
			}
			return;
		}
		_canonicalNames.emplace(p_type.value, std::move(p_canonicalName));
	}

	bool RenderPassBucketPack::contains(const spk::RenderPass::Key &p_key) const noexcept
	{
		return !_built && _index.contains(p_key);
	}

	spk::RenderPass &RenderPassBucketPack::require(const spk::RenderPass::Key &p_key)
	{
		_ensureMutable();
		if (auto iterator = _index.find(p_key); iterator != _index.end())
		{
			return *iterator->second;
		}
		throw std::invalid_argument(
			"Missing render pass type=" + std::to_string(p_key.type.value) +
			", scope=" + std::to_string(p_key.scope.value) +
			", instance=" + std::to_string(p_key.instance));
	}

	const spk::RenderPass &RenderPassBucketPack::require(const spk::RenderPass::Key &p_key) const
	{
		return const_cast<spk::RenderPassBucketPack *>(this)->require(p_key);
	}

	spk::RenderPass *RenderPassBucketPack::find(const spk::RenderPass::Key &p_key) noexcept
	{
		if (_built)
		{
			return nullptr;
		}
		auto iterator = _index.find(p_key);
		return iterator == _index.end() ? nullptr : iterator->second;
	}

	const spk::RenderPass *RenderPassBucketPack::find(const spk::RenderPass::Key &p_key) const noexcept
	{
		return const_cast<spk::RenderPassBucketPack *>(this)->find(p_key);
	}

	std::size_t RenderPassBucketPack::size() const noexcept
	{
		return _passes.size();
	}

	bool RenderPassBucketPack::empty() const noexcept
	{
		return _passes.empty();
	}

	std::vector<spk::RenderPassDiagnostics> RenderPassBucketPack::diagnostics(bool p_sorted) const
	{
		std::vector<const spk::RenderPass *> passes;
		passes.reserve(_passes.size());
		for (const auto &pass : _passes)
		{
			passes.push_back(pass.get());
		}
		if (p_sorted)
		{
			std::stable_sort(passes.begin(), passes.end(), [](const auto *p_lhs, const auto *p_rhs) {
				return p_lhs->priority() != p_rhs->priority()
						   ? p_lhs->priority() < p_rhs->priority()
						   : p_lhs->declarationOrder() < p_rhs->declarationOrder();
			});
		}
		std::vector<spk::RenderPassDiagnostics> result;
		result.reserve(passes.size());
		for (const auto *pass : passes)
		{
			result.push_back(spk::makeRenderPassDiagnostics(*pass));
		}
		return result;
	}

	spk::RenderPlan RenderPassBucketPack::build()
	{
		_ensureMutable();
		_built = true;
		for (auto &pass : _passes)
		{
			pass->_seal();
		}
		std::stable_sort(_passes.begin(), _passes.end(), [](const auto &p_lhs, const auto &p_rhs) {
			return p_lhs->priority() != p_rhs->priority()
					   ? p_lhs->priority() < p_rhs->priority()
					   : p_lhs->declarationOrder() < p_rhs->declarationOrder();
		});
		_index.clear();
		return spk::RenderPlan(std::move(_passes));
	}
}
