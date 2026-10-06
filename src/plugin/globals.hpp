#pragma once

#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp>

#include <chrono>
#include <vector>

#include "../fade.hpp"

inline HANDLE PHANDLE = nullptr;

struct SGlobalState {
    // Event listeners, unregistered explicitly on unload before anything
    // they call into is torn down.
    std::vector<CHyprSignalListener> listeners;
    SP<CEventLoopTimer>              timer;

    struct {
        SP<Config::Values::CBoolValue>   enabled;
        SP<Config::Values::CBoolValue>   blurByDefault;
        SP<Config::Values::CStringValue> blurClass;
        SP<Config::Values::CStringValue> blurTitle;
        SP<Config::Values::CBoolValue>   suppressOnHover;
        SP<Config::Values::CBoolValue>   ensureGlobalBlur;
        SP<Config::Values::CIntValue>    focusLostDelayMs;
        SP<Config::Values::CFloatValue>  fadeRatePercentPerMs;
        SP<Config::Values::CFloatValue>  blurStrength;
    } config;
};

inline UP<SGlobalState> g_pGlobalState;

// Current fade timing from the user settings. Read live on every frame so a
// config reload takes effect without restarting the fade.
HyprBlur::SFadeSettings currentFadeSettings();
bool                    pluginEnabled();
bool                    blurByDefault();
bool                    initialBlurFor(PHLWINDOW window);
bool                    suppressOnHover();
bool                    ensureGlobalBlur();
