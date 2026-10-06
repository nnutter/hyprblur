#pragma once

#include <algorithm>
#include <chrono>

// Timing logic for the blur fade, kept free of compositor types so it can be
// unit tested without a running Hyprland. Time comes from the steady clock
// because wall-clock jumps would otherwise stretch or skip the fade.
namespace HyprBlur {

    struct SFadeSettings {
        // How long after losing focus the fade starts.
        std::chrono::milliseconds focusLostDelay{300};
        // Percentage points of blur added per millisecond after the delay.
        double fadeRatePercentPerMs{0.1};
        // Final blur amount once the fade completes, from 0 (clear) to 1 (full).
        double strength{0.90};
    };

    class CFadeState {
      public:
        using CClock     = std::chrono::steady_clock;
        using CTimePoint = CClock::time_point;

        CFadeState() = default;

        void onFocusLost(CTimePoint at);
        void onFocusGained();

        // Whether the window currently holds focus.
        bool focused() const {
            return m_focused;
        }

        // Current blur amount from 0 (clear) up to the configured strength,
        // rising linearly once the delay has passed.
        double amount(CTimePoint now, const SFadeSettings& settings) const;

        // Whether the fade is still moving, meaning the window needs another
        // frame. True during the delay as well so the compositor stays awake
        // until the fade starts.
        bool animating(CTimePoint now, const SFadeSettings& settings) const;

      private:
        bool       m_focused = true;
        CTimePoint m_focusLostAt{};
    };

    inline void CFadeState::onFocusLost(CTimePoint at) {
        m_focused     = false;
        m_focusLostAt = at;
    }

    inline void CFadeState::onFocusGained() {
        m_focused = true;
    }

    inline double CFadeState::amount(CTimePoint now, const SFadeSettings& settings) const {
        if (m_focused || settings.fadeRatePercentPerMs <= 0.0)
            return 0.0;

        const auto delay = std::max(settings.focusLostDelay, std::chrono::milliseconds::zero());
        if (now - m_focusLostAt < delay)
            return 0.0;

        const double elapsed = std::chrono::duration<double, std::milli>(now - m_focusLostAt - delay).count();
        const double target  = std::clamp(settings.strength, 0.0, 1.0);
        return std::clamp(elapsed * settings.fadeRatePercentPerMs / 100.0, 0.0, target);
    }

    inline bool CFadeState::animating(CTimePoint now, const SFadeSettings& settings) const {
        return !m_focused && settings.fadeRatePercentPerMs > 0.0 && amount(now, settings) < std::clamp(settings.strength, 0.0, 1.0);
    }

} // namespace HyprBlur
