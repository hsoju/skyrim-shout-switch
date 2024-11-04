#include "SwitchManager.h"
#include "Serialization.h"

void SwitchManager::LogFavoritePowers()
{
	for (auto power : switch_powers) {
		logger::info("Power = {}", power->GetName());
	}
}

void SwitchManager::AddFavoritePower(RE::TESForm* power, std::unordered_set<RE::TESForm*>& current_favorites)
{
	if (!switch_set.contains(power)) {
		switch_set.insert(power);
		switch_powers.push_back(power);
	}
	current_favorites.insert(power);
}

std::unordered_set<RE::TESForm*> SwitchManager::GetCurrentFavoritedPowers(RE::BSTArray<RE::TESForm*>& favorite_powers)
{
	std::unordered_set<RE::TESForm*> current_favorites;
	for (auto power_favorite : favorite_powers) {
		auto form_type = power_favorite->GetFormType();
		if (form_type == RE::FormType::Shout) {
			AddFavoritePower(power_favorite, current_favorites);
		} else if (form_type == RE::FormType::Spell) {
			RE::SpellItem* possible_power = power_favorite->As<RE::SpellItem>();
			auto spell_type = possible_power->GetSpellType();
			if (spell_type == RE::MagicSystem::SpellType::kLesserPower || spell_type == RE::MagicSystem::SpellType::kPower) {
				AddFavoritePower(power_favorite, current_favorites);
			}
		}
	}
	return current_favorites;
}

std::vector<RE::TESForm*> SwitchManager::GetUnfavoritedPowers(std::unordered_set<RE::TESForm*>& current_favorites)
{
	std::vector<RE::TESForm*> powers_to_remove;
	for (auto old_power : switch_powers) {
		if (!current_favorites.contains(old_power)) {
			powers_to_remove.push_back(old_power);
		}
	}
	return powers_to_remove;
}

void SwitchManager::RemoveUnfavoritedPowers(std::vector<RE::TESForm*>& powers_to_remove)
{
	for (auto power : powers_to_remove) {
		switch_powers.erase(std::find(switch_powers.begin(), switch_powers.end(), power));
		switch_set.erase(power);
	}
}

void SwitchManager::ProcessFavoritePowers()
{
	RE::BSTArray<RE::TESForm*>& favorite_powers = RE::MagicFavorites::GetSingleton()->spells;
	std::unordered_set<RE::TESForm*> current_favorites = GetCurrentFavoritedPowers(favorite_powers);
	std::vector<RE::TESForm*> powers_to_remove = GetUnfavoritedPowers(current_favorites);
	RemoveUnfavoritedPowers(powers_to_remove);
}

bool SwitchManager::HasSwitchPower(RE::TESForm* power)
{
	return switch_set.contains(power);
}

void SwitchManager::AdvancePower(RE::TESForm* &chosen_power, RE::TESForm* current_power, int& increment)
{
	chosen_power = current_power;
	increment -= 1;
}

void SwitchManager::CheckPowerCooldown(RE::TESForm*& chosen_power, RE::TESForm* current_power, RE::SpellItem* current_spell,
	int& increment)
{
	RE::MagicSystem::CannotCastReason* cast_reason = nullptr;
	RE::Actor* player = RE::PlayerCharacter::GetSingleton()->As<RE::Actor>();
	if (player->CheckCast(current_spell, false, cast_reason)) {
		AdvancePower(chosen_power, current_power, increment);
	}
}

RE::TESForm* SwitchManager::FindNextPower(RE::TESForm* power, int start_idx, int increment)
{
	int current_size = switch_powers.size();
	int current_idx = (start_idx + 1) % current_size;
	RE::TESForm* chosen_power = power;
	int loop_iterations = 0;
	while (increment > 0 && (loop_iterations < current_size)) {
		RE::TESForm* current_power = switch_powers[(current_idx % current_size)];
		if (!ignore_combat_powers.contains(current_power) || !RE::PlayerCharacter::GetSingleton()->IsInCombat()) {
			RE::FormType current_type = current_power->GetFormType();
			if (current_type == RE::FormType::Spell && (enable_lesser_powers || enable_powers)) {
				RE::SpellItem* current_spell = current_power->As<RE::SpellItem>();
				RE::MagicSystem::SpellType current_type = current_spell->GetSpellType();
				if (current_type == RE::MagicSystem::SpellType::kLesserPower && enable_lesser_powers) {
					AdvancePower(chosen_power, current_power, increment);
				} else if (current_type == RE::MagicSystem::SpellType::kPower && enable_powers) {
					CheckPowerCooldown(chosen_power, current_power, current_spell, increment);
				}
			} else if (current_type == RE::FormType::Shout && enable_shouts) {
				AdvancePower(chosen_power, current_power, increment);
			}
		}
		current_idx = (current_idx + 1) % current_size;
		loop_iterations += 1;
	}
	return chosen_power;
}

RE::TESForm* SwitchManager::FindNextPowerUsingIncrement(RE::TESForm* power, int start_idx, int increment) {
	RE::TESForm* possible_power = FindNextPower(power, start_idx, increment);
	if (possible_power == power) {
		possible_power = FindNextPower(power, start_idx, 1);
	}
	return possible_power;
}

RE::TESForm* SwitchManager::GetNextPower(RE::TESForm* power, int increment) {
	if (!HasSwitchPower(power)) {
		return power;
	}
	auto iter = std::find(switch_powers.begin(), switch_powers.end(), power);
	if (iter != switch_powers.end()) {
		int idx = iter - switch_powers.begin();
		return FindNextPowerUsingIncrement(power, idx, increment);
	}
	return power;
}

void SwitchManager::ImportIgnoreCombatPowers(std::list<CSimpleIniA::Entry>& ignore_powers)
{
	for (auto& entry : ignore_powers) {
		auto editor_id = std::string(entry.pItem);
		auto given_form = RE::TESForm::LookupByEditorID(editor_id);
		if (given_form) {
			ignore_combat_powers.insert(given_form);
			logger::info("{}: added to Ignore in Combat list", editor_id);
		}
	}
}

void SwitchManager::ImportRecastPowers(std::list<CSimpleIniA::Entry>& custom_powers)
{
	for (auto& entry : custom_powers) {
		auto editor_id = std::string(entry.pItem);
		auto given_power = RE::TESForm::LookupByEditorID<RE::SpellItem>(editor_id);
		if (given_power) {
			recast_powers[given_power] = 0;
			logger::info("{}: added to Recast list", editor_id);
		}
	}
}

void SwitchManager::ImportSettings() {
	std::lock_guard<std::shared_mutex> lk(settings_mtx);
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\ShoutSwitch.ini");

	enable_shouts = ini.GetBoolValue("Settings", "bEnableShouts", true);
	enable_lesser_powers = ini.GetBoolValue("Settings", "bEnableLesserPowers", true);
	enable_powers = ini.GetBoolValue("Settings", "bEnablePowers", true);

	disable_out_of_combat = ini.GetBoolValue("Settings", "bOnlyInCombat", false);

	std::list<CSimpleIniA::Entry> custom_powers;
	ini.GetAllKeys("RecastLesserPowers", custom_powers);
	ImportRecastPowers(custom_powers);

	std::list<CSimpleIniA::Entry> ignore_powers;
	ini.GetAllKeys("SkipInCombat", ignore_powers);
	ImportRecastPowers(ignore_powers);
}

bool SwitchManager::SerializeSave(SKSE::SerializationInterface* a_intfc)
{
	power_storage.clear();
	for (int idx = 0; idx < switch_powers.size(); idx++) {
		auto powerID = switch_powers[idx]->formID;
		power_storage.push_back(std::to_string(powerID));
	}

	if (!Serialization::Save(a_intfc, this->power_storage)) {
		logger::error("Failed to write power switches");
		return false;
	}
	return true;
}

bool SwitchManager::SerializeSave(SKSE::SerializationInterface* a_intfc, uint32_t a_type, uint32_t a_version)
{
	if (!a_intfc->OpenRecord(a_type, a_version)) {
		logger::error("Failed to open power switch records!");
		return false;
	}
	return SerializeSave(a_intfc);
}

bool SwitchManager::DeserializeLoad(SKSE::SerializationInterface* a_intfc)
{
	power_storage.clear();
	if (!Serialization::Load(a_intfc, this->power_storage)) {
		logger::info("Failed to load power switches!");
		return false;
	}

	switch_powers.clear();
	switch_set.clear();
	for (auto& item : power_storage.items()) {
		std::string power_sID = item.value().get<std::string>();
		auto power = RE::TESForm::LookupByID(static_cast<RE::FormID>(std::stoul(power_sID)));
		switch_powers.push_back(power);
		switch_set.insert(power);
	}

	return true;
}

void SwitchManager::Revert()
{
	switch_powers.clear();
	switch_set.clear();
}
