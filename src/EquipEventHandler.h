#pragma once

class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	static bool Register()
	{
		static EquipEventHandler singleton;

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