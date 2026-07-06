#include "world/generator/worldgen_params.hpp"

#include "core/json.hpp"

#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace
{
	void requireFinite(float p_value, const char *p_name)
	{
		if (!std::isfinite(p_value))
		{
			throw std::invalid_argument(std::string("worldgen parameter '") + p_name + "' must be finite");
		}
	}
}

namespace pg
{
	WorldgenParams WorldgenParams::parse(const JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "worldSize", "landmass", "height", "cities", "biomes", "transport"});

		WorldgenParams result;
		result.version = p_reader.require<int>("version");
		result.worldSize = p_reader.require<std::array<int, 2>>("worldSize");
		result.biomes = p_reader.require<std::vector<std::string>>("biomes");

		const JsonReader landmass = p_reader.child("landmass");
		landmass.forbidUnknown({"falloff", "regionCount", "landmassCount", "landRatio", "borderOceanWidth", "compactness", "noiseFrequency", "noiseAmplitude", "seaLevel", "detailFrequency", "detailAmplitude"});
		result.landmass.falloff = landmass.require<std::string>("falloff");
		result.landmass.regionCount = landmass.require<int>("regionCount");
		result.landmass.landmassCount = landmass.require<std::array<int, 2>>("landmassCount");
		result.landmass.landRatio = landmass.require<float>("landRatio");
		result.landmass.borderOceanWidth = landmass.require<int>("borderOceanWidth");
		result.landmass.compactness = landmass.require<float>("compactness");
		result.landmass.noiseFrequency = landmass.require<float>("noiseFrequency");
		result.landmass.noiseAmplitude = landmass.require<float>("noiseAmplitude");
		result.landmass.seaLevel = landmass.require<float>("seaLevel");
		result.landmass.detailFrequency = landmass.require<float>("detailFrequency");
		result.landmass.detailAmplitude = landmass.require<float>("detailAmplitude");

		const JsonReader height = p_reader.child("height");
		height.forbidUnknown({"baseFrequency", "mountainFrequency", "mountainThreshold", "maxHeight"});
		result.height.baseFrequency = height.require<float>("baseFrequency");
		result.height.mountainFrequency = height.require<float>("mountainFrequency");
		result.height.mountainThreshold = height.require<float>("mountainThreshold");
		result.height.maxHeight = height.require<int>("maxHeight");

		const JsonReader cities = p_reader.child("cities");
		cities.forbidUnknown({"majorCount", "minSpacing", "coastalBias", "satellitesPerCity", "satelliteRadius"});
		result.cities.majorCount = cities.require<int>("majorCount");
		result.cities.minSpacing = cities.require<float>("minSpacing");
		result.cities.coastalBias = cities.require<float>("coastalBias");
		result.cities.satellitesPerCity = cities.require<std::array<int, 2>>("satellitesPerCity");
		result.cities.satelliteRadius = cities.require<float>("satelliteRadius");

		const JsonReader transport = p_reader.child("transport");
		transport.forbidUnknown({"extraEdges", "slopeCostFactor", "riverCrossingCost", "tunnelTriggerCost"});
		result.transport.extraEdges = transport.require<int>("extraEdges");
		result.transport.slopeCostFactor = transport.require<float>("slopeCostFactor");
		result.transport.riverCrossingCost = transport.require<float>("riverCrossingCost");
		result.transport.tunnelTriggerCost = transport.require<float>("tunnelTriggerCost");

		if (result.version != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported version (expected 1)");
		}
		try
		{
			result.validate();
		} catch (const std::invalid_argument &exception)
		{
			throw JsonError(p_reader.file(), p_reader.path(), exception.what());
		}
		return result;
	}

	WorldgenParams WorldgenParams::load(const std::filesystem::path &p_file)
	{
		const nlohmann::json json = JsonLoader::parseFile(p_file);
		return parse(JsonReader(json, p_file));
	}

	void WorldgenParams::validate() const
	{
		if (version != 1)
		{
			throw std::invalid_argument("worldgen version must be 1");
		}
		if (worldSize[0] < 32 || worldSize[1] < 32)
		{
			throw std::invalid_argument("worldSize dimensions must both be at least 32");
		}
		if (landmass.falloff != "macroVoronoi")
		{
			throw std::invalid_argument("landmass.falloff must be 'macroVoronoi'");
		}
		if (landmass.regionCount < 16)
		{
			throw std::invalid_argument("landmass.regionCount must be at least 16");
		}
		if (landmass.landmassCount[0] < 1 || landmass.landmassCount[1] < landmass.landmassCount[0])
		{
			throw std::invalid_argument("landmass.landmassCount must be a positive ordered range");
		}
		requireFinite(landmass.landRatio, "landmass.landRatio");
		requireFinite(landmass.compactness, "landmass.compactness");
		if (landmass.landRatio <= 0.05f || landmass.landRatio >= 0.9f)
		{
			throw std::invalid_argument("landmass.landRatio must be in ]0.05, 0.9[");
		}
		if (landmass.borderOceanWidth < 1 || landmass.compactness < 0.0f)
		{
			throw std::invalid_argument("landmass border width must be positive and compactness cannot be negative");
		}
		if (landmass.borderOceanWidth * 2 >= std::min(worldSize[0], worldSize[1]))
		{
			throw std::invalid_argument("landmass.borderOceanWidth leaves no usable world area");
		}
		requireFinite(landmass.noiseFrequency, "landmass.noiseFrequency");
		requireFinite(landmass.noiseAmplitude, "landmass.noiseAmplitude");
		requireFinite(landmass.seaLevel, "landmass.seaLevel");
		requireFinite(landmass.detailFrequency, "landmass.detailFrequency");
		requireFinite(landmass.detailAmplitude, "landmass.detailAmplitude");
		if (landmass.noiseFrequency <= 0.0f || landmass.detailFrequency <= 0.0f)
		{
			throw std::invalid_argument("landmass noise frequencies must be positive");
		}
		if (landmass.noiseAmplitude < 0.0f || landmass.detailAmplitude < 0.0f)
		{
			throw std::invalid_argument("landmass noise amplitudes cannot be negative");
		}
		requireFinite(height.baseFrequency, "height.baseFrequency");
		requireFinite(height.mountainFrequency, "height.mountainFrequency");
		requireFinite(height.mountainThreshold, "height.mountainThreshold");
		if (height.baseFrequency <= 0.0f || height.mountainFrequency <= 0.0f || height.maxHeight < 2)
		{
			throw std::invalid_argument("height frequencies must be positive and maxHeight must be at least 2");
		}
		if (height.mountainThreshold < 0.0f || height.mountainThreshold > 1.0f)
		{
			throw std::invalid_argument("height.mountainThreshold must be in [0, 1]");
		}
		requireFinite(cities.minSpacing, "cities.minSpacing");
		requireFinite(cities.coastalBias, "cities.coastalBias");
		requireFinite(cities.satelliteRadius, "cities.satelliteRadius");
		if (cities.majorCount < 2 || cities.minSpacing < 1.0f || cities.satelliteRadius < 1.0f)
		{
			throw std::invalid_argument("city count must be at least 2 and city distances must be positive");
		}
		if (cities.majorCount < landmass.landmassCount[1])
		{
			throw std::invalid_argument("cities.majorCount must allow at least one port town per landmass");
		}
		if (cities.coastalBias < 0.0f || cities.coastalBias > 1.0f)
		{
			throw std::invalid_argument("cities.coastalBias must be in [0, 1]");
		}
		if (cities.satellitesPerCity[0] < 0 || cities.satellitesPerCity[1] < cities.satellitesPerCity[0])
		{
			throw std::invalid_argument("cities.satellitesPerCity must be a non-negative ordered range");
		}
		if (biomes.empty())
		{
			throw std::invalid_argument("biomes must contain at least one entry");
		}
		std::unordered_set<std::string> uniqueBiomes;
		for (const std::string &biome : biomes)
		{
			if (biome.empty() || !uniqueBiomes.insert(biome).second)
			{
				throw std::invalid_argument("biomes must contain unique non-empty identifiers");
			}
		}
		if (uniqueBiomes.size() == 1 && uniqueBiomes.contains("coast"))
		{
			throw std::invalid_argument("biomes must contain at least one non-coast biome");
		}
		if (transport.extraEdges < 0)
		{
			throw std::invalid_argument("transport.extraEdges cannot be negative");
		}
		requireFinite(transport.slopeCostFactor, "transport.slopeCostFactor");
		requireFinite(transport.riverCrossingCost, "transport.riverCrossingCost");
		requireFinite(transport.tunnelTriggerCost, "transport.tunnelTriggerCost");
		if (transport.slopeCostFactor < 0.0f || transport.riverCrossingCost < 0.0f || transport.tunnelTriggerCost <= 0.0f)
		{
			throw std::invalid_argument("transport costs must be non-negative and tunnelTriggerCost must be positive");
		}
	}
}
