#pragma once

namespace pg
{
	// Exact three-column cross used to realize roads and town paths inside a plan
	// cell. Keeping this geometry shared prevents roadside decor from drifting away
	// from the surface that PlanChunkProvider actually paints.
	[[nodiscard]] inline bool isCenteredPathSurface(
		int p_blocksPerCell,
		int p_localX,
		int p_localZ,
		bool p_north,
		bool p_south,
		bool p_west,
		bool p_east) noexcept
	{
		const int strip = p_blocksPerCell / 2 - 1;
		const bool inStripX = p_localX >= strip && p_localX <= strip + 2;
		const bool inStripZ = p_localZ >= strip && p_localZ <= strip + 2;
		if (inStripX && inStripZ)
		{
			return true;
		}
		if (inStripX && ((p_north && p_localZ <= strip + 2) || (p_south && p_localZ >= strip)))
		{
			return true;
		}
		return inStripZ && ((p_west && p_localX <= strip + 2) || (p_east && p_localX >= strip));
	}
}
