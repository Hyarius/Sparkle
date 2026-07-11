#pragma once

#include "structures/container/spk_json_reader.hpp"

namespace pg
{
	// Promoted into the library (structures/container/spk_json_reader.hpp) on
	// 2026-07-11; these aliases keep the many pg parser call sites unchanged.
	using JsonError = spk::JSON::Error;
	using JsonLoader = spk::JSON::Loader;
	using JsonReader = spk::JSON::Reader;
}
