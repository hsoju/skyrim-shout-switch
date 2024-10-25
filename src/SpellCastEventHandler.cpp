#include "SpellCastEventHandler.h"

RE::BGSEquipSlot* GetVoiceSlot()
{
	using func_t = decltype(&GetVoiceSlot);
	const REL::Relocation<func_t> func{ RELOCATION_ID(23153, 23610) };
	return func();
}

void SpellCastEventHandler::EquipPower(RE::Actor* player, RE::TESForm* power)
{
	auto power_type = power->GetFormType();
	if (power_type == RE::FormType::Shout) {
		RE::TESShout* shout = power->As<RE::TESShout>();
		RE::ActorEquipManager::GetSingleton()->EquipShout(player, shout);
	} else {
		RE::SpellItem* voice_power = power->As<RE::SpellItem>();
		RE::ActorEquipManager::GetSingleton()->EquipSpell(player, voice_power, GetVoiceSlot());
	}
}

void HandleShoutCooldown(RE::TESObjectREFR* caster)
{
	RE::Actor* player = caster->As<RE::Actor>();
	auto& runtimeData = player->GetActorRuntimeData();
	auto* process = runtimeData.currentProcess;
	if (process) {
		auto* high_data = process->high;
		if (high_data) {
			auto recovery = high_data->voiceRecoveryTime;
		}
	}
}

void SpellCastEventHandler::HandleShout(RE::TESObjectREFR* caster, RE::SpellItem* casted_power)
{
	auto manager = SwitchManager::GetSingleton();
	if (manager->enable_shouts)
	{
		RE::Actor* player = caster->As<RE::Actor>();
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
				RE::TESForm* switched_power = SwitchManager::GetSingleton()->GetNextPower(shout, current_shout_idx + 1);
				if (switched_power != shout) {
					EquipPower(player, switched_power);
				}
			}
		}
	}
}

void SpellCastEventHandler::HandlePower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power)
{
	auto manager = SwitchManager::GetSingleton();
	if (manager->enable_powers) {
		RE::TESForm* switched_power = SwitchManager::GetSingleton()->GetNextPower(casted_power, 1);
		if (switched_power != casted_power) {
			RE::Actor* player = caster->As<RE::Actor>();
			EquipPower(player, switched_power);
		}
	}
}

void SpellCastEventHandler::HandleLesserPower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power)
{
	RE::TESForm* switched_power = SwitchManager::GetSingleton()->GetNextPower(casted_power, 1);
	if (switched_power != casted_power) {
		RE::Actor* player = caster->As<RE::Actor>();
		EquipPower(player, switched_power);
	}
}

void SpellCastEventHandler::CheckLesserPower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power)
{
	auto manager = SwitchManager::GetSingleton();
	if (manager->enable_lesser_powers) {
		if (!manager->recast_powers.contains(casted_power)) {
			HandleLesserPower(caster, casted_power);
		} else {
			manager->recast_powers[casted_power] = manager->recast_powers[casted_power] + 1;
			if (manager->recast_powers[casted_power] >= 2) {
				manager->recast_powers[casted_power] = 0;
				HandleLesserPower(caster, casted_power);
			}
		}
	}
}
