#include "SpellCastEventHandler.h"

RE::BGSEquipSlot* GetVoiceSlot()
{
	using func_t = decltype(&GetVoiceSlot);
	const REL::Relocation<func_t> func{ RELOCATION_ID(23153, 23610) };
	return func();
}

void SpellCastEventHandler::EquipPower(RE::Actor* player, RE::TESForm* current_power, RE::TESForm* chosen_power)
{
	if (current_power != chosen_power) {
		auto power_type = chosen_power->GetFormType();
		if (power_type == RE::FormType::Shout) {
			RE::TESShout* shout = chosen_power->As<RE::TESShout>();
			RE::ActorEquipManager::GetSingleton()->EquipShout(player, shout);
		} else {
			RE::SpellItem* voice_power = chosen_power->As<RE::SpellItem>();
			RE::ActorEquipManager::GetSingleton()->EquipSpell(player, voice_power, GetVoiceSlot());
		}
	}
}

void SpellCastEventHandler::HandleShout(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager)
{
	if (manager->enable_shouts)
	{
		RE::TESShout* shout = player->GetCurrentShout();
		if (shout) {
			auto shout_variations = shout->variations;
			int current_shout_idx = -1;
			for (int idx = 0; idx < 3; idx++) {
				if (shout_variations[idx].spell == casted_power) {
					current_shout_idx = idx;
					break;
				}
			}
			if (current_shout_idx >= 0) {
				EquipPower(player, shout, manager->GetNextPower(shout, current_shout_idx + 1));
			}
		}
	}
}

void SpellCastEventHandler::HandlePower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager)
{
	if (manager->enable_powers) {
		EquipPower(player, casted_power, manager->GetNextPower(casted_power, 1));
	}
}

void SpellCastEventHandler::HandleLesserPower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager)
{
	EquipPower(player, casted_power, manager->GetNextPower(casted_power, 1));
}

void SpellCastEventHandler::CheckLesserPower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager)
{
	if (manager->enable_lesser_powers) {
		if (!manager->recast_powers.contains(casted_power)) {
			HandleLesserPower(player, casted_power, manager);
		} else {
			manager->recast_powers[casted_power] = manager->recast_powers[casted_power] + 1;
			if (manager->recast_powers[casted_power] >= 2) {
				manager->recast_powers[casted_power] = 0;
				HandleLesserPower(player, casted_power, manager);
			}
		}
	}
}
