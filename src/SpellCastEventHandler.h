#pragma once

#include "InputEventHandler.h"
#include "MenuHandler.h"
#include "SwitchManager.h"

class SpellCastEventHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
{
public:
	void EquipPower(RE::Actor* player, RE::TESForm* power);

	void HandleShout(RE::TESObjectREFR* caster, RE::SpellItem* casted_power);
	void HandlePower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power);
	void CheckLesserPower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power);
	void HandleLesserPower(RE::TESObjectREFR* caster, RE::SpellItem* casted_power);

	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
	{
		auto caster = a_event->object.get();
		if (!caster || !caster->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event || !a_event->spell) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::SpellItem* casted_spell = RE::TESForm::LookupByID<RE::SpellItem>(a_event->spell);
		if (casted_spell) {
			auto player = caster->As<RE::Actor>();
			if (SwitchManager::GetSingleton()->disable_out_of_combat && !player->IsInCombat()) {
				return RE::BSEventNotifyControl::kContinue;
			}
			auto spell_type = casted_spell->GetSpellType();
			if (spell_type == RE::MagicSystem::SpellType::kVoicePower) {
				HandleShout(caster, casted_spell);
			} else {
				if (casted_spell == player->GetActorRuntimeData().selectedPower) {
					if (spell_type == RE::MagicSystem::SpellType::kLesserPower) {
						HandleLesserPower(caster, casted_spell);
					} else if (spell_type == RE::MagicSystem::SpellType::kPower) {
						HandlePower(caster, casted_spell);
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
