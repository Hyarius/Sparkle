#pragma once

#include "structures/container/spk_definition_registry.hpp"

namespace pg
{
	template <typename TDefinition>
	using Registry = spk::DefinitionRegistry<TDefinition>;
}
