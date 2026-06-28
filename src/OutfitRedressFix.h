#pragma once

namespace OutfitRedressFix
{
	struct Set3DUpdateFlag
	{
		static float thunk(RE::AIProcess* a_process, RE::RESET_3D_FLAGS a_flags)
		{
			a_flags = RE::RESET_3D_FLAGS::kModel;
			return func(a_process, a_flags);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline bool Install()
	{
		auto& trampoline = REL::GetTrampoline();
		const REL::Relocation<uintptr_t> target(REL::ID{ 323782, 2230394 }, REL::Offset{ 0x29, 0x1E });
		Set3DUpdateFlag::func = trampoline.write_call<5>(target.address(), Set3DUpdateFlag::thunk);

		return true;
	}
}
