#pragma once

#include <SimpleIni.h>
#undef ERROR

namespace OutfitRedressFix
{
	namespace outfitRedressFixDetail
	{
		inline std::unordered_set<RE::TESFormID> exclusions;

		inline std::string& Trim(std::string& String) noexcept
		{
			constexpr static char whitespaceDelimiters[] = " \t\n\r\f\v";

			if (!String.empty())
			{
				String.erase(String.find_last_not_of(whitespaceDelimiters) + 1);
				String.erase(0, String.find_first_not_of(whitespaceDelimiters));
			}

			return String;
		}

		inline bool GetLoadOrderByFormID(RE::TESDataHandler* dataHandler, const char* a_pluginName, uint32_t& a_formID) noexcept
		{
			__try
			{
				if (a_pluginName && a_pluginName[0])
				{
					std::optional<uint16_t> id = dataHandler->GetLoadedModIndex(a_pluginName);
					if (!id.has_value())
					{
						id = dataHandler->GetLoadedLightModIndex(a_pluginName);
						if (!id.has_value())
						{
							REX::WARN("Found no Plugin for '{}' for Form ID 0x{:08X}"sv, a_pluginName, a_formID);
							return false;
						}

						a_formID = (a_formID & (0x00000FFF)) | (*id << 12) | 0xFE000000;
					}
					else
						a_formID = (a_formID & (0x00FFFFFF)) | (*id << 24);
				}
				else
				{
					REX::WARN("Empty Plugin Name for Form ID 0x{:08X}"sv, a_formID);
					return false;
				}

				return true;
			}
			__except (1)
			{
				REX::WARN("Failed to find Load Order Form ID for '{}:0x{:08X}'"sv, a_pluginName, a_formID);
				return false;
			}
		}

		inline void ReadExceptions()
		{
			const auto exclusionsFile = "Data\\F4SE\\Plugins\\OutfitRedressFixAE_Exceptions.ini";

			// Get TESDataHandler
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler)
			{
				REX::WARN("ReadExceptions: Could not get TESDataHandler.");
				return;
			}

			// Load INI
			CSimpleIniA ini;
			if (ini.LoadFile(exclusionsFile) != SI_OK)
			{
				REX::WARN("ReadExceptions: Exception File '{}' was not found.", exclusionsFile);
				return;
			}

			// Get Exceptions Section
			const auto* section = ini.GetSection("Exceptions");
			if (!section)
			{
				REX::WARN("ReadExceptions: Section [Exceptions] was not found.");
				return;
			}

			// Get all Keys
			for (const auto& key : *section)
			{
				std::uint32_t formID = 0;
				std::string keyValue = key.second;
				Trim(keyValue);

				if (keyValue.empty() || !keyValue.length())
					continue;

				auto It = keyValue.find_first_of(':');
				if (It == std::string::npos)
					continue;

				std::string pluginName = keyValue.substr(It + 1);
				std::string value = keyValue.substr(0, It);
				Trim(pluginName);
				Trim(value);

				if (pluginName.empty() || !pluginName.length() ||
					value.empty() || !value.length())
					continue;

				if (value.find_first_of("0x"sv) == 0)
					formID = strtoul(value.c_str() + 2, nullptr, 16);
				else
					formID = strtoul(value.c_str(), nullptr, 10);

				if (GetLoadOrderByFormID(dataHandler, pluginName.c_str(), formID))
				{
					REX::INFO("ReadExceptions: Exception added '{}:0x{:08X}'."sv, pluginName, formID);
					exclusions.insert(formID);
				}
			}
		}

		struct RedressIfNeeded
		{
			static void thunk(RE::Actor* a_actor, bool a_dontAddOutfit)
			{
				if (!a_actor)
					return;

				auto* process = a_actor->currentProcess;
				if (!process || process->processLevel > 1)
					return;

				REX::TEnumSet flags{ RE::RESET_3D_FLAGS::kModel };

				// Vanilla Behavior for Exclusions
				const auto* npc = a_actor->GetNPC();
				const bool isExcluded = npc && exclusions.contains(npc->GetFormID());
				if (isExcluded)
				{
					flags.set(RE::RESET_3D_FLAGS::kInitDefault);
					if (a_dontAddOutfit)
						flags.set(RE::RESET_3D_FLAGS::kDontAddOutfit);
				}

				return a_actor->Set3DUpdateFlag((RE::RESET_3D_FLAGS)flags.underlying());
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	inline bool Install()
	{
		auto& trampoline = REL::GetTrampoline();

		const REL::Relocation<std::uintptr_t> target{ REL::ID{ 323782, 2230394 } };
		outfitRedressFixDetail::RedressIfNeeded::func = trampoline.write_jmp<5>(target.address(), outfitRedressFixDetail::RedressIfNeeded::thunk);

		return true;
	}

	inline void F4SEMessageListener(F4SE::MessagingInterface::Message* a_msg)
	{
		if (a_msg && a_msg->type == F4SE::MessagingInterface::kGameDataReady)
			outfitRedressFixDetail::ReadExceptions();
	}
}
