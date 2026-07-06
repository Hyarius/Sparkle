#pragma once

#include "core/registries.hpp"
#include "tools/core/json_writer.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace pg::tools
{
	class ContentDocument
	{
	public:
		enum class Domain
		{
			Encounter,
			Ability,
			Status,
			Species
		};

	private:
		Domain _domain = Domain::Ability;
		std::filesystem::path _file;
		std::filesystem::path _sourceFile;
		std::string _id;
		nlohmann::json _json;
		bool _dirty = false;

	public:
		void load(const std::filesystem::path &p_file, Domain p_domain);
		void create(
			const std::filesystem::path &p_dataDirectory,
			std::string p_id,
			Domain p_domain,
			const Registries &p_registries);
		void save(const JsonWriter &p_writer);

		[[nodiscard]] Domain domain() const noexcept;
		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const std::string &id() const noexcept;
		[[nodiscard]] const nlohmann::json &json() const noexcept;
		[[nodiscard]] nlohmann::json &data() noexcept;
		[[nodiscard]] nlohmann::json &editJson() noexcept;
		[[nodiscard]] bool dirty() const noexcept;
		void rename(const std::string &p_id);

		[[nodiscard]] std::vector<std::string> validate(const Registries &p_registries) const;
		[[nodiscard]] std::string rulesPreview(const Registries &p_registries) const;

		[[nodiscard]] static std::string directoryName(Domain p_domain);
		[[nodiscard]] static std::vector<std::string> effectTypes();
		[[nodiscard]] static nlohmann::json effectDefaults(const std::string &p_type);
		[[nodiscard]] static std::string completedNodesSummary(
			const std::string &p_speciesId,
			const nlohmann::json &p_completedNodes,
			const Registries &p_registries);
	};
}
