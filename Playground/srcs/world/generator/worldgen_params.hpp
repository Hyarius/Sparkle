#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;

	struct WorldgenParams
	{
		struct Landmass
		{
			std::string falloff = "macroVoronoi";
			int regionCount = 450;
			std::array<int, 2> landmassCount{1, 3};
			float landRatio = 0.48f;
			int borderOceanWidth = 8;
			float compactness = 0.35f;
			float noiseFrequency = 0.004f;
			float noiseAmplitude = 0.35f;
			float seaLevel = 0.0f;
			float detailFrequency = 0.02f;
			float detailAmplitude = 0.1f;
		};

		struct Height
		{
			float baseFrequency = 0.008f;
			float mountainFrequency = 0.02f;
			float mountainThreshold = 0.65f;
			int maxHeight = 48;
		};

		struct Cities
		{
			int majorCount = 8;
			float minSpacing = 90.0f;
			float coastalBias = 0.25f;
			std::array<int, 2> satellitesPerCity{1, 3};
			float satelliteRadius = 60.0f;
		};

		struct Transport
		{
			int extraEdges = 3;
			float slopeCostFactor = 4.0f;
			float riverCrossingCost = 25.0f;
			float tunnelTriggerCost = 400.0f;
		};

		int version = 1;
		std::array<int, 2> worldSize{1024, 1024};
		Landmass landmass;
		Height height;
		Cities cities;
		std::vector<std::string> biomes;
		Transport transport;

		[[nodiscard]] static WorldgenParams parse(const JsonReader &p_reader);
		[[nodiscard]] static WorldgenParams load(const std::filesystem::path &p_file);
		void validate() const;
	};
}
