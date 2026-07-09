#include "world/generator/placement_rules.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "world/prefab_definition.hpp"

#include <array>
#include <utility>

namespace pg
{
	const std::array<std::pair<const char *, PlanEntityKind>, 6> &planEntityKeyTable() noexcept
	{
		static const std::array<std::pair<const char *, PlanEntityKind>, 6> table = {{
			{"gym", PlanEntityKind::Gym},
			{"city", PlanEntityKind::City},
			{"portCity", PlanEntityKind::PortCity},
			{"rarePoi", PlanEntityKind::RarePoi},
			{"uncommonPoi", PlanEntityKind::UncommonPoi},
			{"normalPoi", PlanEntityKind::NormalPoi},
		}};
		return table;
	}

	std::vector<std::string> parsePrefabPool(
		const spk::JSON::Value &p_value,
		const std::filesystem::path &p_file,
		const std::string &p_path,
		const Registry<PrefabDefinition> &p_prefabs)
	{
		std::vector<std::string> pool;
		const auto append = [&](const spk::JSON::Value &p_entry, const std::string &p_entryPath) {
			if (!p_entry.isString())
			{
				throw JsonError(p_file, p_entryPath, "expected a prefab id string");
			}
			const std::string prefabId = p_entry.as<std::string>();
			if (p_prefabs.tryGet(prefabId) == nullptr)
			{
				throw JsonError(p_file, p_entryPath, "unknown prefab id '" + prefabId + "'");
			}
			pool.push_back(prefabId);
		};
		if (p_value.isArray())
		{
			const spk::JSON::Value::Array &entries = p_value.asArray();
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				append(entries[index], p_path + "[" + std::to_string(index) + "]");
			}
		}
		else
		{
			append(p_value, p_path);
		}
		return pool;
	}

	PlanPlacementRules parsePlanPlacementRules(JsonReader &p_reader, const Registry<PrefabDefinition> &p_prefabs)
	{
		p_reader.forbidUnknown({"version", "entities"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported placements version");
		}

		// Stairways are not configured here; each biome builds them from its palette
		// stair/slope voxels (see synthesizeClimbPrefabs), and the world plan generator
		// picks among the resulting per-biome flight pools.
		PlanPlacementRules rules;

		const JsonReader entitiesReader = p_reader.child("entities");
		entitiesReader.forbidUnknown({"gym", "city", "portCity", "rarePoi", "uncommonPoi", "normalPoi"});
		for (const auto &[key, kind] : planEntityKeyTable())
		{
			if (!entitiesReader.contains(key))
			{
				continue;
			}
			const spk::JSON::Value &value = *entitiesReader.value().find(key);
			if (value.isNull())
			{
				continue; // explicit "no prefab for this kind"
			}
			std::vector<std::string> pool =
				parsePrefabPool(value, p_reader.file(), entitiesReader.pathFor(key), p_prefabs);
			if (!pool.empty())
			{
				rules.entityPrefabs.emplace(kind, std::move(pool));
			}
		}
		return rules;
	}
}
