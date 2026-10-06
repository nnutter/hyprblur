#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "globals.hpp"
#include "tracker.hpp"
#include "BlurDeco.hpp"

static void onNewWindow(PHLWINDOW window) {
    if (!window)
        return;

    g_pTracker->onWindowOpened(window);

    for (const auto& deco : window->m_windowDecorations) {
        if (deco->getDisplayName() == "hyprblur")
            return;
    }

    HyprlandAPI::addWindowDecoration(PHANDLE, window, Hyprutils::Memory::makeUnique<CBlurDeco>(window));
}

// The plugin entry points below return C++ types through C linkage. That is
// how the Hyprland plugin API is shaped, so silence the same warning the API
// headers themselves silence around their declarations.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

// Do NOT change this function.
// NOLINTNEXTLINE(modernize-use-string-view): signature mandated by the Hyprland plugin API.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

HyprBlur::SFadeSettings currentFadeSettings() {
    HyprBlur::SFadeSettings settings;
    settings.focusLostDelay       = std::chrono::milliseconds(g_pGlobalState->config.focusLostDelayMs->value());
    settings.fadeRatePercentPerMs = static_cast<double>(g_pGlobalState->config.fadeRatePercentPerMs->value());
    settings.strength             = static_cast<double>(g_pGlobalState->config.blurStrength->value());
    return settings;
}

bool pluginEnabled() {
    return g_pGlobalState->config.enabled->value();
}

bool blurByDefault() {
    return g_pGlobalState->config.blurByDefault->value();
}

bool suppressOnHover() {
    return g_pGlobalState->config.suppressOnHover->value();
}

bool ensureGlobalBlur() {
    return g_pGlobalState->config.ensureGlobalBlur->value();
}

// Only fixed messages from this file are passed here, never window titles
// or config text. Omarchy's command marks these as user-action notifications,
// which remain visible in Do Not Disturb mode. The shell runs asynchronously
// and falls back to desktop notifications, then Hyprland's overlay.
static void notifyToggle(std::string_view message) {
    const auto command = std::format("omarchy notification send -t 3000 '{}' || notify-send --app-name=hyprblur --urgency=low --expire-time=3000 '{}' || "
                                     "hyprctl notify -1 3000 'rgb(99ccff)' '{}'",
                                     message, message, message);

    // With Lua config, dispatch expects a Lua expression, not "exec ...".
    // Use the public eval command to reach Lua's asynchronous process launcher.
    auto result = HyprlandAPI::invokeHyprctlCommand("eval", std::format("hl.exec_cmd([=[{}]=])", command));
    if (result == "eval is only supported with the lua config manager")
        result = HyprlandAPI::invokeHyprctlCommand("dispatch", std::format("exec {}", command));

    if (result != "ok") {
        Log::logger->log(Log::WARN, "[hyprblur] notification launch failed: {}", result);
        HyprlandAPI::addNotification(PHANDLE, std::string{message}, CHyprColor{0.6, 0.8, 1.0, 1.0}, 3000);
    }
}

static void reportToggle(const CBlurTracker::SToggleResult& toggled) {
    Log::logger->log(Log::INFO, "[hyprblur] blur {} for '{}'", toggled.enabled ? "enabled" : "disabled", toggled.title);
    notifyToggle(toggled.enabled ? "blur enabled" : "blur disabled");
}

static bool globalBlurEnabled() {
    // Bound fresh on every call: a cached handle can outlive the value it
    // points at across config reloads.
    const auto PBLURGLOBAL = CConfigValue<Config::BOOL>("decoration:blur:enabled");
    return PBLURGLOBAL.good() && *PBLURGLOBAL;
}

// Refuse the toggle when there is no blur to show through. Opting the
// window in anyway would look exactly like success until focus changes.
static std::string activeWindowTitle() {
    const auto active = Desktop::focusState()->window();
    return active ? active->fetchTitle() : std::string{"unknown"};
}

// Keep diagnostic state in the log rather than desktop notifications.
static void reportStatus() {
    const std::string text =
        std::format("[hyprblur]\nglobal blur: {}\nplugin: {}\n{}", globalBlurEnabled() ? "on" : "off", pluginEnabled() ? "on" : "off", g_pTracker->statusText());
    Log::logger->log(Log::INFO, "{}", text);
}

static int statusLuaFunction(lua_State*) {
    reportStatus();
    return 0;
}

// Lua entry point: hl.plugin.hyprblur.toggle(). Classic dispatchers cannot
// be named from Lua, so this is how Lua-config users toggle windows.
static int toggleLuaFunction(lua_State*) {
    if (!globalBlurEnabled()) {
        Log::logger->log(Log::WARN, "[hyprblur] toggle refused for '{}': global blur is off", activeWindowTitle());
        notifyToggle("blur unavailable, disabled globally");
        return 0;
    }

    if (!pluginEnabled()) {
        Log::logger->log(Log::WARN, "[hyprblur] toggle refused: plugin is disabled");
        notifyToggle("blur unavailable, plugin disabled");
        return 0;
    }

    const auto toggled = g_pTracker->toggleActiveWindow();
    if (!toggled) {
        Log::logger->log(Log::WARN, "[hyprblur] toggle ignored: no active window");
        notifyToggle("blur unavailable, no active window");
        return 0;
    }

    reportToggle(*toggled);
    return 0;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprblur] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprblur] Version mismatch");
    }

    g_pGlobalState = Hyprutils::Memory::makeUnique<SGlobalState>();

    g_pGlobalState->config.enabled = Hyprutils::Memory::makeShared<Config::Values::CBoolValue>("plugin:hyprblur:enabled", "Blur windows after they lose focus", true);
    g_pGlobalState->config.blurByDefault =
        Hyprutils::Memory::makeShared<Config::Values::CBoolValue>("plugin:hyprblur:blur_by_default", "Blur newly opened windows once they lose focus", false);
    g_pGlobalState->config.suppressOnHover =
        Hyprutils::Memory::makeShared<Config::Values::CBoolValue>("plugin:hyprblur:suppress_on_hover", "Keep a window clear while the pointer hovers over it", true);
    g_pGlobalState->config.ensureGlobalBlur =
        Hyprutils::Memory::makeShared<Config::Values::CBoolValue>("plugin:hyprblur:ensure_global_blur", "Ensure global blur is on after each config reload", true);
    g_pGlobalState->config.focusLostDelayMs = Hyprutils::Memory::makeShared<Config::Values::CIntValue>(
        "plugin:hyprblur:focus_lost_delay_ms", "Delay after losing focus before the blur starts fading in", 300, Config::Values::SIntValueOptions{.min = 0});
    g_pGlobalState->config.fadeRatePercentPerMs =
        Hyprutils::Memory::makeShared<Config::Values::CFloatValue>("plugin:hyprblur:fade_rate_percent_per_ms", "Percentage points of blur added per millisecond", 0.1F,
                                                                   Config::Values::SFloatValueOptions{.min = std::numeric_limits<float>::min()});
    g_pGlobalState->config.blurStrength = Hyprutils::Memory::makeShared<Config::Values::CFloatValue>(
        "plugin:hyprblur:blur_strength", "Final blur amount from 0 (clear) to 1 (full)", 0.90F, Config::Values::SFloatValueOptions{.min = 0.F, .max = 1.F});

    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.enabled);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.blurByDefault);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.suppressOnHover);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.ensureGlobalBlur);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.focusLostDelayMs);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.fadeRatePercentPerMs);
    HyprlandAPI::addConfigValueV2(PHANDLE, g_pGlobalState->config.blurStrength);

    g_pTracker = Hyprutils::Memory::makeUnique<CBlurTracker>();

    g_pGlobalState->listeners.push_back(Event::bus()->m_events.window.open.listen([&](PHLWINDOW w) { onNewWindow(w); }));
    g_pGlobalState->listeners.push_back(Event::bus()->m_events.window.active.listen([&](PHLWINDOW w, Desktop::eFocusReason) { g_pTracker->onWindowActive(w); }));
    g_pGlobalState->listeners.push_back(Event::bus()->m_events.window.close.listen([&](PHLWINDOW w) { g_pTracker->onWindowClosed(w); }));
    g_pGlobalState->listeners.push_back(Event::bus()->m_events.window.destroy.listen([&](PHLWINDOWREF w) { g_pTracker->onWindowClosed(w.lock()); }));
    // Hyprland's animation tick stops on idle desktops. Keep fade timing
    // and pointer-hover reconciliation independent of its animations.
    g_pGlobalState->timer = Hyprutils::Memory::makeShared<CEventLoopTimer>(
        std::chrono::milliseconds{16},
        [](SP<CEventLoopTimer> timer, void*) {
            g_pTracker->onTick();
            timer->updateTimeout(std::chrono::milliseconds{16});
        },
        nullptr);
    g_pEventLoopManager->addTimer(g_pGlobalState->timer);

    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprblur:toggle", [&](const std::string&) {
        SDispatchResult result;
        if (!globalBlurEnabled()) {
            result.success = false;
            result.error   = "global decoration:blur:enabled is off";
            Log::logger->log(Log::WARN, "[hyprblur] toggle refused for '{}': global blur is off", activeWindowTitle());
            notifyToggle("blur unavailable, disabled globally");
            return result;
        }

        if (!pluginEnabled()) {
            result.success = false;
            result.error   = "plugin is disabled";
            Log::logger->log(Log::WARN, "[hyprblur] toggle refused: plugin is disabled");
            notifyToggle("blur unavailable, plugin disabled");
            return result;
        }

        const auto toggled = g_pTracker->toggleActiveWindow();
        if (!toggled) {
            result.success = false;
            result.error   = "no active window";
            Log::logger->log(Log::WARN, "[hyprblur] toggle ignored: no active window");
            notifyToggle("blur unavailable, no active window");
            return result;
        }

        reportToggle(*toggled);
        return result;
    });
    HyprlandAPI::addLuaFunction(PHANDLE, "hyprblur", "toggle", toggleLuaFunction);
    HyprlandAPI::addLuaFunction(PHANDLE, "hyprblur", "status", statusLuaFunction);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprblur:status", [&](const std::string&) {
        reportStatus();
        return SDispatchResult{};
    });

    for (const auto& w : Desktop::windowState()->windows()) {
        if (w->isHidden() || !Desktop::View::validMapped(w))
            continue;

        onNewWindow(w);
    }

    // Apply after every reload so desktop defaults cannot silently disable
    // plugin blur. Users can opt out with ensure_global_blur=false; runtime
    // changes are left alone until the next config reload.
    g_pGlobalState->listeners.push_back(Event::bus()->m_events.config.reloaded.listen([] {
        if (!ensureGlobalBlur())
            return;

        const auto PBLURGLOBAL = CConfigValue<Config::BOOL>("decoration:blur:enabled");
        if (PBLURGLOBAL.good() && !*PBLURGLOBAL) {
            *PBLURGLOBAL.ptr() = true;
            Log::logger->log(Log::INFO, "[hyprblur] global decoration:blur:enabled was off, turned it on");
        }
    }));

    // Hyprland's plugin loader schedules the reload after PLUGIN_INIT.
    return {.name = "hyprblur", .description = "Blur a window a moment after it loses focus, fading the blur in.", .author = "Nathan Nutter", .version = "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // Unregister first so no event can reach this module while the rest
    // comes down.
    g_pGlobalState->timer->cancel();
    g_pEventLoopManager->removeTimer(g_pGlobalState->timer);
    g_pGlobalState->timer.reset();
    g_pGlobalState->listeners.clear();
    HyprlandAPI::removeDispatcher(PHANDLE, "hyprblur:toggle");
    HyprlandAPI::removeDispatcher(PHANDLE, "hyprblur:status");

    for (const auto& w : Desktop::windowState()->windows()) {
        std::vector<IHyprWindowDecoration*> ours;
        for (const auto& deco : w->m_windowDecorations) {
            if (deco->getDisplayName() == "hyprblur")
                ours.push_back(deco.get());
        }
        for (auto* deco : ours)
            HyprlandAPI::removeWindowDecoration(PHANDLE, deco);
    }

    g_pHyprRenderer->m_renderPass.removeAllOfType("CBlurPassElement");

    g_pTracker.reset();
    g_pGlobalState.reset();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
