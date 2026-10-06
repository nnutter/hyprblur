#include "tracker.hpp"

#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/helpers/time/Time.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include <algorithm>
#include <format>

#include "globals.hpp"

namespace {
    HyprBlur::CFadeState::CTimePoint now() {
        return Time::steadyNow();
    }
} // namespace

CBlurTracker::SEntry* CBlurTracker::find(PHLWINDOW window) {
    for (auto& entry : m_entries) {
        if (entry.window.lock() == window)
            return &entry;
    }
    return nullptr;
}

CBlurTracker::SEntry& CBlurTracker::ensure(PHLWINDOW window) {
    if (auto* found = find(window))
        return *found;

    m_entries.push_back(SEntry{.window = window, .fade = {}, .optedIn = blurByDefault()});
    return m_entries.back();
}

void CBlurTracker::onWindowOpened(PHLWINDOW window) {
    if (window)
        ensure(window);
}

void CBlurTracker::onWindowActive(PHLWINDOW window) {
    const auto at = now();

    for (auto& entry : m_entries) {
        const auto owner = entry.window.lock();
        if (!owner)
            continue;

        if (owner == window) {
            if (!entry.fade.focused()) {
                entry.fade.onFocusGained();
                damage(owner);
            }
        } else if (entry.fade.focused()) {
            entry.fade.onFocusLost(at);
            damage(owner);
        }
    }

    if (window)
        ensure(window);
}

void CBlurTracker::onWindowClosed(PHLWINDOW window) {
    std::erase_if(m_entries, [&](const SEntry& entry) { return entry.window.lock() == window; });
}

void CBlurTracker::onTick() {
    const auto at       = now();
    const auto settings = currentFadeSettings();

    std::erase_if(m_entries, [](const SEntry& entry) { return entry.window.expired(); });

    for (auto& entry : m_entries) {
        const auto owner = entry.window.lock();
        if (!owner)
            continue;

        const bool focused = focusedNow(owner);
        if (focused != entry.fade.focused()) {
            if (focused)
                entry.fade.onFocusGained();
            else
                entry.fade.onFocusLost(at);
            damage(owner);
        } else if (entry.fade.animating(at, settings)) {
            damage(owner);
        }
    }
}

std::optional<CBlurTracker::SToggleResult> CBlurTracker::toggleActiveWindow() {
    const auto active = Desktop::focusState()->window();
    if (!active)
        return std::nullopt;

    auto& entry   = ensure(active);
    entry.optedIn = !entry.optedIn;
    if (!entry.optedIn)
        entry.fade.onFocusGained();
    damage(active);
    return SToggleResult{.enabled = entry.optedIn, .title = active->fetchTitle()};
}

double CBlurTracker::amountFor(PHLWINDOW window) {
    if (!pluginEnabled() || !window)
        return 0.0;

    const auto* entry = find(window);
    if (!entry || !entry->optedIn)
        return 0.0;

    return entry->fade.amount(now(), currentFadeSettings());
}

std::string CBlurTracker::statusText() {
    const auto  settings = currentFadeSettings();

    std::string text = std::format("tracked: {}", m_entries.size());
    for (const auto& entry : m_entries) {
        const auto owner = entry.window.lock();
        if (!owner)
            continue;

        std::string title = owner->fetchTitle();
        if (title.size() > 24)
            title.replace(24, std::string::npos, "...");

        bool hasDeco = false;
        for (const auto& deco : owner->m_windowDecorations) {
            if (deco->getDisplayName() == "hyprblur") {
                hasDeco = true;
                break;
            }
        }

        text += std::format("\n'{}': {}, {}, amount {:.2f}{}", title, entry.optedIn ? "opted in" : "opted out", entry.fade.focused() ? "focused" : "unfocused",
                            entry.fade.amount(now(), settings), hasDeco ? "" : ", NO DECO");
    }
    return text;
}

void CBlurTracker::damage(PHLWINDOW window) {
    if (window)
        g_pHyprRenderer->damageWindow(window);
}

bool CBlurTracker::focusedNow(PHLWINDOW window) {
    if (window == Desktop::focusState()->window())
        return true;

    return suppressOnHover() && pointerOver(window);
}

bool CBlurTracker::pointerOver(PHLWINDOW window) {
    return window->getWindowMainSurfaceBox().containsPoint(g_pInputManager->getMouseCoordsInternal());
}
