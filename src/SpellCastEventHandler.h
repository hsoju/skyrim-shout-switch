#pragma once

#include "InputEventHandler.h"
#include "MenuHandler.h"
#include "SwitchManager.h"

class SpellCastEventHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
{
public:
	void EquipPower(RE::Actor* player, RE::TESForm* current_power, RE::TESForm* chosen_power);

	void HandleShout(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager);
	void HandlePower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager);
	void CheckLesserPower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager);
	void HandleLesserPower(RE::Actor* player, RE::SpellItem* casted_power, SwitchManager* manager);

	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
	{
		RE::TESObjectREFR* caster = a_event->object.get();
		if (!caster || !caster->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event || !a_event->spell) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::SpellItem* casted_spell = RE::TESForm::LookupByID<RE::SpellItem>(a_event->spell);
		if (casted_spell) {
			RE::Actor* player = caster->As<RE::Actor>();
			SwitchManager* manager = SwitchManager::GetSingleton();
			if (manager->disable_out_of_combat && !player->IsInCombat()) {
				return RE::BSEventNotifyControl::kContinue;
			}
			RE::MagicSystem::SpellType spell_type = casted_spell->GetSpellType();
			if (spell_type == RE::MagicSystem::SpellType::kVoicePower) {
				HandleShout(player, casted_spell, manager);
			} else {
				if (casted_spell == player->GetActorRuntimeData().selectedPower) {
					if (spell_type == RE::MagicSystem::SpellType::kLesserPower) {
						HandleLesserPower(player, casted_spell, manager);
					} else if (spell_type == RE::MagicSystem::SpellType::kPower) {
						HandlePower(player, casted_spell, manager);
					}
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	static bool Register()
	{
		static SpellCastEventHandler singleton;
		auto ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();

		if (!ScriptEventSource) {
			logger::error("Script event source not found");
			return false;
		}

		ScriptEventSource->AddEventSink(&singleton);

		logger::info("Registered {}", typeid(singleton).name());

		return true;
	}
};
