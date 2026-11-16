#pragma once

#include "ModBase.h"
#include "vrshim/Bridge.h" // FRIK-flat C12

namespace f4cf::main_hook
{
    namespace internal
    {
        // Hook into a random function call inside the game loop function
        // By overriding the address of one of the calls we can inject our own code to run
        // on every frame.
#ifdef FRIK_FLAT
        // C12: flat drives the frame from the PlayerUpdateEvent notify at the end of Main::OnIdle_UpdatePlayer (flat call
        // C351F3, VR D84F2C = F4VROffsets hookEndUpdate): on flat the third-person body is still animated in first person
        // (shadows), so a frame hook early in OnIdle (VR D8405E -> flat C33E62) would have FRIK's pose overwritten by the
        // animation update. Bridge v2: FRIK-flat no longer patches that call itself; fo4flatvr owns one byte-checked hook
        // there (FrikBridge.cpp, chained with its floating gun) and calls the callback registered below. Kept for the log.
        using HookFunc = int (*)(uint64_t rcx, uint64_t rdx);
        inline REL::Relocation _hookFuncCallSite{ vrshim::FlatOffset(0xd84f2c) };
#else
        using HookFunc = void (*)(uint64_t rcx);
        inline REL::Relocation _hookFuncCallSite{ vrshim::FlatOffset(0xd8405e) };
#endif
        inline HookFunc _hookFuncOriginal = nullptr;
        inline uintptr_t _hookFuncThisHook = 0;

        /**
         * Read the destination of a CALL at address
         */
        static std::uintptr_t getCallTarget(const std::uintptr_t address)
        {
            const std::int32_t rel = *reinterpret_cast<std::int32_t*>(address + 1); // NOLINT(performance-no-int-to-ptr)
            return address + 5 + rel;
        }

        /**
         * Hook handler inside game frame update tick.
         */
#ifdef FRIK_FLAT
        // bridge v2 frame callback (fo4flatvr's C351F3 hook, before its floating gun and the original notify)
        inline void onBridgeFrame(void*) noexcept
        {
            vrshim::bridge::frameUpdate();
            g_mod->onFrameUpdateSafe();
        }
#else
        inline void onGameFrameUpdateHook(const std::uint64_t rcx)
        {
            g_mod->onFrameUpdateSafe();
            _hookFuncOriginal(rcx);
        }
#endif
    }

    /**
     * Hook into the main game loop by overriding a function call inside it.
     */
    inline void hook()
    {
        logger::info("Hook into main loop at (0x{:X})...", internal::_hookFuncCallSite.address());
#ifdef FRIK_FLAT // C12 (bridge v2): fo4flatvr owns the call site; register the frame callback instead of patching
        if (vrshim::bridge::registerFrame(&internal::onBridgeFrame, nullptr)) {
            logger::info("Main loop: frame callback registered with fo4flatvr's C351F3 hook (no FRIK-side patch)");
        } else {
            logger::error("Main loop: fo4flatvr's C351F3 hook is not installed (byte check failed or bridge missing): FRIK-flat frame update NOT running");
        }
#else
        auto& trampoline = F4SE::GetTrampoline();
        const auto hookFuncOriginal = trampoline.write_call<5>(internal::_hookFuncCallSite.address(), &internal::onGameFrameUpdateHook);

        internal::_hookFuncThisHook = internal::getCallTarget(internal::_hookFuncCallSite.address());
        logger::info("Hook into main loop successful, original: (0x{:X}), new: (0x{:X})", hookFuncOriginal, internal::_hookFuncThisHook);
        internal::_hookFuncOriginal = reinterpret_cast<internal::HookFunc>(hookFuncOriginal); // NOLINT(performance-no-int-to-ptr)
#endif
    }

    /**
     * Validate that the hook was NOT overridden by another mod.
     * It is possible that another mod will override the hook but also respect the latest set address like this code dose.
     * In that case both mods will run on every frame. The warning is to help debugging if the other mods is not written well.
     */
    inline void validate()
    {
#ifndef FRIK_FLAT  // bridge v2: no FRIK-side patch to validate
        const auto current = internal::getCallTarget(internal::_hookFuncCallSite.address());
        if (current != internal::_hookFuncThisHook) {
            logger::warn("Main Loop Hook validation failed! Expected: (0x{:X}), Current: (0x{:X})", internal::_hookFuncThisHook, current);
        }
#endif
    }
}
