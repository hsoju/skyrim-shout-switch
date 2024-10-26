#include "SpellCastEventHandler.h"
#include "InputEventHandler.h"
#include "MenuHandler.h"

bool SetupSettings() {
	SwitchManager::GetSingleton()->ImportSettings();
	return true;
}

bool SetupSpellCastHandler()
{
	SpellCastEventHandler::Register();
	return true;
}

bool SetupFavoritesTracker()
{
	MenuHandler::Register();
	InputEventHandler::Register();
	return true;
}