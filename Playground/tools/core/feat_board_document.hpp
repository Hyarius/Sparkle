#pragma once

#include "core/registries.hpp"
#include "tools/core/json_writer.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sparkle.hpp>

namespace pg::tools
{
	struct FeatBoardValidationIssue
	{
		enum class Code
		{
			EmptyBoard,
			InvalidUuid,
			DuplicateUuid,
			MissingRoot,
			DanglingNeighbour,
			AsymmetricNeighbour,
			DisconnectedGraph,
			DanglingAbility,
			DanglingStatus,
			FormTierOrder,
			ParseError
		};

		Code code = Code::ParseError;
		std::string message;
		bool fixable = false;
	};

	class FeatBoardDocument
	{
	private:
		std::filesystem::path _file;
		std::filesystem::path _sourceFile;
		std::string _id;
		nlohmann::json _json;
		std::string _selectedNode;
		bool _dirty = false;

		[[nodiscard]] nlohmann::json *_node(const std::string &p_uuid);
		[[nodiscard]] const nlohmann::json *_node(const std::string &p_uuid) const;

	public:
		void load(const std::filesystem::path &p_file);
		void create(const std::filesystem::path &p_directory, std::string p_id);
		void save(const JsonWriter &p_writer);

		[[nodiscard]] const std::string &id() const noexcept;
		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const nlohmann::json &json() const noexcept;
		[[nodiscard]] nlohmann::json &editJson() noexcept;
		[[nodiscard]] bool dirty() const noexcept;
		void markChanged() noexcept;
		void rename(const std::string &p_id);

		[[nodiscard]] const std::string &selectedNode() const noexcept;
		void selectNode(const std::string &p_uuid);
		[[nodiscard]] nlohmann::json *selectedNodeJson();
		[[nodiscard]] const nlohmann::json *selectedNodeJson() const;
		[[nodiscard]] std::string addNode(const spk::Vector2 &p_position);
		bool deleteNode(const std::string &p_uuid);
		bool moveNode(const std::string &p_uuid, const spk::Vector2 &p_position);
		bool toggleLink(const std::string &p_first, const std::string &p_second);
		std::string addRequirement(const std::string &p_type = "dealDamage");
		void removeRequirement(std::size_t p_index);
		void setRequirementType(std::size_t p_index, const std::string &p_type);
		void addReward(const std::string &p_type = "bonusStats");
		void removeReward(std::size_t p_index);
		void setRewardType(std::size_t p_index, const std::string &p_type);

		[[nodiscard]] static std::vector<std::string> requirementTypes();
		[[nodiscard]] static std::vector<std::string> rewardTypes();
		[[nodiscard]] static nlohmann::json requirementDefaults(const std::string &p_type);
		[[nodiscard]] static nlohmann::json rewardDefaults(const std::string &p_type);
		[[nodiscard]] static std::vector<FeatBoardValidationIssue> validate(
			const nlohmann::json &p_json,
			const std::string &p_boardId,
			const Registries &p_registries);
		[[nodiscard]] static std::size_t fixNeighbourSymmetry(nlohmann::json &p_json);
		[[nodiscard]] static bool reorderFormNodes(
			nlohmann::json &p_json,
			const std::string &p_boardId,
			const Registries &p_registries);
		[[nodiscard]] std::vector<std::string> linkedSpecies(const Registries &p_registries) const;
	};
}
