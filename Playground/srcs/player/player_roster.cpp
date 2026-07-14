#include "player/player_roster.hpp"

#include "core/registries.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace pg
{
	namespace
	{
		[[noreturn]] void fail(const std::string &p_message)
		{
			throw std::invalid_argument("roster: " + p_message);
		}

		[[nodiscard]] std::string idText(CreatureInstanceId p_id)
		{
			return p_id.valid() ? std::string(p_id.string()) : std::string("<invalid>");
		}
	}

	std::optional<std::size_t> PlayerRoster::_teamSlotOf(CreatureInstanceId p_id) const noexcept
	{
		for (std::size_t slot = 0; slot < TeamCapacity; ++slot)
		{
			if (_team[slot].has_value() && _team[slot]->id == p_id)
			{
				return slot;
			}
		}
		return std::nullopt;
	}

	std::optional<std::size_t> PlayerRoster::_storageIndexOf(CreatureInstanceId p_id) const noexcept
	{
		for (std::size_t index = 0; index < _storage.size(); ++index)
		{
			if (_storage[index].id == p_id)
			{
				return index;
			}
		}
		return std::nullopt;
	}

	const PlayerRoster::Team &PlayerRoster::team() const noexcept
	{
		return _team;
	}

	const std::vector<CreatureUnit> &PlayerRoster::storage() const noexcept
	{
		return _storage;
	}

	const CreatureUnit *PlayerRoster::find(CreatureInstanceId p_id) const noexcept
	{
		if (!p_id.valid())
		{
			return nullptr;
		}
		if (const std::optional<std::size_t> slot = _teamSlotOf(p_id); slot.has_value())
		{
			return &_team[*slot].value();
		}
		if (const std::optional<std::size_t> index = _storageIndexOf(p_id); index.has_value())
		{
			return &_storage[*index];
		}
		return nullptr;
	}

	CreatureUnit *PlayerRoster::findMutable(CreatureInstanceId p_id) noexcept
	{
		return const_cast<CreatureUnit *>(std::as_const(*this).find(p_id));
	}

	bool PlayerRoster::contains(CreatureInstanceId p_id) const noexcept
	{
		return find(p_id) != nullptr;
	}

	std::size_t PlayerRoster::size() const noexcept
	{
		std::size_t total = _storage.size();
		for (const std::optional<CreatureUnit> &member : _team)
		{
			total += member.has_value() ? 1U : 0U;
		}
		return total;
	}

	PlayerRoster::Placement PlayerRoster::add(CreatureUnit p_unit)
	{
		if (!p_unit.id.valid())
		{
			fail("a creature needs a valid instance id");
		}
		if (contains(p_unit.id))
		{
			fail("already holds '" + idText(p_unit.id) + "'");
		}

		for (std::size_t slot = 0; slot < TeamCapacity; ++slot)
		{
			if (!_team[slot].has_value())
			{
				_team[slot] = std::move(p_unit);
				return Placement{.inTeam = true, .index = slot};
			}
		}

		_storage.push_back(std::move(p_unit));
		return Placement{.inTeam = false, .index = _storage.size() - 1};
	}

	PlayerRoster::Placement PlayerRoster::moveTeamToStorage(CreatureInstanceId p_id)
	{
		const std::optional<std::size_t> slot = _teamSlotOf(p_id);
		if (!slot.has_value())
		{
			fail("no team member '" + idText(p_id) + "'");
		}

		_storage.push_back(std::move(_team[*slot].value()));
		_team[*slot].reset();
		return Placement{.inTeam = false, .index = _storage.size() - 1};
	}

	PlayerRoster::Placement PlayerRoster::moveStorageToTeam(CreatureInstanceId p_id, std::size_t p_slot)
	{
		if (p_slot >= TeamCapacity)
		{
			fail("team slot " + std::to_string(p_slot) + " does not exist");
		}
		if (_team[p_slot].has_value())
		{
			fail("team slot " + std::to_string(p_slot) + " is occupied; swap or free it first");
		}

		const std::optional<std::size_t> index = _storageIndexOf(p_id);
		if (!index.has_value())
		{
			fail("no stored creature '" + idText(p_id) + "'");
		}

		_team[p_slot] = std::move(_storage[*index]);
		// Erase keeps the remaining insertion order, which is the box's whole contract.
		_storage.erase(_storage.begin() + static_cast<std::ptrdiff_t>(*index));
		return Placement{.inTeam = true, .index = p_slot};
	}

	void PlayerRoster::swapTeamSlots(std::size_t p_first, std::size_t p_second)
	{
		if (p_first >= TeamCapacity || p_second >= TeamCapacity)
		{
			fail(
				"team slots " + std::to_string(p_first) + " and " + std::to_string(p_second) +
				" must both be below " + std::to_string(TeamCapacity));
		}
		std::swap(_team[p_first], _team[p_second]);
	}

	void PlayerRoster::swapTeamAndStorage(CreatureInstanceId p_teamMember, CreatureInstanceId p_stored)
	{
		const std::optional<std::size_t> slot = _teamSlotOf(p_teamMember);
		if (!slot.has_value())
		{
			fail("no team member '" + idText(p_teamMember) + "'");
		}
		const std::optional<std::size_t> index = _storageIndexOf(p_stored);
		if (!index.has_value())
		{
			fail("no stored creature '" + idText(p_stored) + "'");
		}

		// In place on both sides: the stored creature takes the exact box position of the one it
		// replaces, so a swap never reshuffles the box.
		std::swap(_team[*slot].value(), _storage[*index]);
	}

	CreatureUnit PlayerRoster::remove(CreatureInstanceId p_id)
	{
		if (const std::optional<std::size_t> slot = _teamSlotOf(p_id); slot.has_value())
		{
			CreatureUnit removed = std::move(_team[*slot].value());
			_team[*slot].reset();
			return removed;
		}
		if (const std::optional<std::size_t> index = _storageIndexOf(p_id); index.has_value())
		{
			CreatureUnit removed = std::move(_storage[*index]);
			_storage.erase(_storage.begin() + static_cast<std::ptrdiff_t>(*index));
			return removed;
		}
		fail("holds no creature '" + idText(p_id) + "'");
	}

	void PlayerRoster::validate(const DerivationContext &p_context) const
	{
		if (p_context.species == nullptr || p_context.featBoards == nullptr)
		{
			throw std::invalid_argument("derivation context is missing a registry");
		}

		std::set<std::uint64_t> serials;
		const auto check = [&p_context, &serials](const CreatureUnit &p_unit) {
			if (!p_unit.id.valid())
			{
				fail("holds a creature with no instance id");
			}
			if (!serials.insert(p_unit.id.serial()).second)
			{
				fail("holds '" + idText(p_unit.id) + "' twice");
			}

			const CreatureSpeciesDefinition *species = p_context.species->tryGet(p_unit.speciesId);
			if (species == nullptr)
			{
				fail("'" + idText(p_unit.id) + "' is of unknown species '" + p_unit.speciesId + "'");
			}
			const FeatBoardDefinition *board = p_context.featBoards->tryGet(species->featBoardId);
			if (board == nullptr)
			{
				fail("species '" + p_unit.speciesId + "' selects unknown feat board '" + species->featBoardId + "'");
			}

			// The cache is a cache: it has to equal what the persistent half derives, or a save
			// could carry stats no board ever granted.
			if (deriveCreatureState(*species, *board, p_unit.featProgress, p_context) != p_unit.derived)
			{
				fail("'" + idText(p_unit.id) + "' carries a derived state its Feat progress does not derive");
			}
		};

		for (const std::optional<CreatureUnit> &member : _team)
		{
			if (member.has_value())
			{
				check(*member);
			}
		}
		for (const CreatureUnit &stored : _storage)
		{
			check(stored);
		}
	}

	void PlayerRoster::validate(const Registries &p_registries) const
	{
		validate(derivationContextOf(p_registries));
	}
}
