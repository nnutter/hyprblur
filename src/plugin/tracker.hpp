#pragma once

#include <hyprland/src/desktop/DesktopTypes.hpp>

#include <optional>
#include <string>
#include <vector>

#include "../fade.hpp"

// Remembers every window, whether it opted into blurring, and how far its
// fade has progressed. Focus changes arrive as events for immediacy while
// the per-frame tick reconciles pointer hover and keeps the fade animating.
class CBlurTracker {
  public:
    CBlurTracker() = default;

    void onWindowOpened(PHLWINDOW window);
    void onWindowActive(PHLWINDOW window);
    void onWindowClosed(PHLWINDOW window);
    void onTick();

    // Flip blurring for the active window. Empty when there is no window.
    struct SToggleResult {
        bool        enabled = false;
        std::string title;
    };
    std::optional<SToggleResult> toggleActiveWindow();

    // Current blur amount for a window, or 0 when it should stay clear.
    // Side-effect free so the renderer can call it while drawing.
    double amountFor(PHLWINDOW window);

    // Human-readable snapshot of the live state for the status reporter.
    std::string statusText();

  private:
    struct SEntry {
        PHLWINDOWREF         window;
        HyprBlur::CFadeState fade;
        bool                 optedIn = true;
    };

    std::vector<SEntry> m_entries;

    SEntry*             find(PHLWINDOW window);
    SEntry&             ensure(PHLWINDOW window);
    void                damage(PHLWINDOW window);
    bool                focusedNow(PHLWINDOW window);
    bool                pointerOver(PHLWINDOW window);
};

inline UP<CBlurTracker> g_pTracker;
