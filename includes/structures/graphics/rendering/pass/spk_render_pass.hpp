#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/rendering/pass/spk_render_contribution.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_clear.hpp"
#include "structures/graphics/rendering/pass/spk_render_target_reference.hpp"

namespace spk
{
	class RenderPass
	{
	public:
		struct TypeId
		{
			std::uint64_t value = 0;

			auto operator<=>(const TypeId &) const = default;
		};

		struct ScopeId
		{
			std::uint64_t value = 0;

			auto operator<=>(const ScopeId &) const = default;

			[[nodiscard]] static ScopeId generate() noexcept
			{
				static std::atomic<std::uint64_t> next{1};
				return {next.fetch_add(1, std::memory_order_relaxed)};
			}
		};

		struct Key
		{
			TypeId type;
			ScopeId scope;
			std::uint32_t instance = 0;

			auto operator<=>(const Key &) const = default;
		};
		
		struct Description
		{
			std::string debugName;
			spk::RenderTargetReference target;
			spk::RenderPassClear clear;
		};
	private:
		Key _key;
		std::string _debugName;

		Description _description;
		std::int32_t _priority = 0;
		std::size_t _declarationOrder = 0;
		std::vector<spk::RenderContribution> _contributions;
		bool _sealed = false;
		bool _commandsTaken = false;

		void _seal();
		void _add(std::size_t p_contribution, std::unique_ptr<spk::RenderCommand> p_command);
		friend class spk::RenderContributionSink;
		friend class RenderPassBucketPack;

	public:
		RenderPass(
			Key p_key,
			std::int32_t p_priority,
			std::size_t p_declarationOrder,
			std::string p_debugName,
			Description p_description);
		virtual ~RenderPass() = default;

		RenderPass(const RenderPass &) = delete;
		RenderPass &operator=(const RenderPass &) = delete;
		RenderPass(RenderPass &&) noexcept = default;
		RenderPass &operator=(RenderPass &&) noexcept = delete;

		[[nodiscard]] const Key &key() const noexcept;
		[[nodiscard]] const std::string &debugName() const noexcept;
		[[nodiscard]] const Description &description() const noexcept;
		[[nodiscard]] std::int32_t priority() const noexcept;
		void setPriority(std::int32_t p_priority);
		[[nodiscard]] std::size_t declarationOrder() const noexcept;
		[[nodiscard]] std::size_t contributorCount() const noexcept;
		[[nodiscard]] std::size_t commandCount() const noexcept;

		[[nodiscard]] spk::RenderContributionSink contribute(
			std::int32_t p_contributorPriority,
			std::size_t p_registrationOrder);

		[[nodiscard]] std::vector<std::unique_ptr<spk::RenderCommand>> takeCommands();
	};

	void validateRenderPassDescription(const spk::RenderPass::Description &p_description);

	[[nodiscard]] consteval RenderPass::TypeId makeRenderPassTypeId(std::string_view p_name)
	{
		std::uint64_t hash = 14695981039346656037ull;
		for (const char character : p_name)
		{
			hash ^= static_cast<unsigned char>(character);
			hash *= 1099511628211ull;
		}
		return {hash};
	}
}

template <>
struct std::hash<spk::RenderPass::Key>
{
	std::size_t operator()(const spk::RenderPass::Key &p_key) const noexcept
	{
		std::size_t result = std::hash<std::uint64_t>{}(p_key.type.value);
		result ^= std::hash<std::uint64_t>{}(p_key.scope.value) + 0x9e3779b9 + (result << 6) + (result >> 2);
		result ^= std::hash<std::uint32_t>{}(p_key.instance) + 0x9e3779b9 + (result << 6) + (result >> 2);
		return result;
	}
};

template <>
struct std::hash<spk::RenderPass::TypeId>
{
	std::size_t operator()(const spk::RenderPass::TypeId &p_id) const noexcept
	{
		return std::hash<std::uint64_t>{}(p_id.value);
	}
};