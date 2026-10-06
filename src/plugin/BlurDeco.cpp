#include "BlurDeco.hpp"

#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include "tracker.hpp"

CBlurDeco::CBlurDeco(PHLWINDOW window) : IHyprWindowDecoration(window), m_window(window) {
    ;
}

SDecorationPositioningInfo CBlurDeco::getPositioningInfo() {
    // The overlay floats over the window without changing its layout.
    SDecorationPositioningInfo info;
    info.policy         = DECORATION_POSITION_ABSOLUTE;
    info.edges          = 0;
    info.priority       = 0;
    info.desiredExtents = {};
    info.reserved       = false;
    return info;
}

void CBlurDeco::onPositioningReply(const SDecorationPositioningReply&) {
    // The box is read live from the window on every frame.
}

void CBlurDeco::draw(PHLMONITOR monitor, float const& a) {
    const auto window = m_window.lock();
    if (!window || !monitor)
        return;

    const double amount = g_pTracker->amountFor(window);

    // Transitions only, so the log shows when each window starts and stops
    // blurring without spamming every frame in between.
    if (amount > 0.0 && m_lastReportedAmount <= 0.0)
        Log::logger->log(Log::INFO, "[hyprblur] blur started for '{}'", window->fetchTitle());
    else if (amount <= 0.0 && m_lastReportedAmount > 0.0)
        Log::logger->log(Log::INFO, "[hyprblur] blur cleared for '{}'", window->fetchTitle());
    m_lastReportedAmount = amount;

    if (amount <= 0.0)
        return;

    g_pHyprRenderer->m_renderPass.add(Hyprutils::Memory::makeUnique<CBlurPassElement>(CBlurPassElement::SBlurData{.deco = this, .amount = static_cast<float>(amount * a)}));
}

eDecorationType CBlurDeco::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CBlurDeco::updateWindow(PHLWINDOW window) {
    m_window = window;
    damageEntire();
}

void CBlurDeco::damageEntire() {
    const auto window = m_window.lock();
    if (window)
        g_pHyprRenderer->damageWindow(window);
}

eDecorationLayer CBlurDeco::getDecorationLayer() {
    // Draw after the main surface. Inactive tiled windows do not get the
    // popup pass where Hyprland draws OVERLAY decorations.
    return DECORATION_LAYER_OVER;
}

uint64_t CBlurDeco::getDecorationFlags() {
    return 0;
}

std::string CBlurDeco::getDisplayName() {
    return "hyprblur";
}

CBox CBlurDeco::windowBoxOnMonitor(PHLMONITOR monitor) {
    const auto window = m_window.lock();
    if (!window || !monitor)
        return {};

    CBox box = window->getWindowMainSurfaceBox();
    box.translate(-monitor->m_position);
    return box;
}

void CBlurDeco::renderPass(PHLMONITOR monitor, float amount) {
    const auto window = m_window.lock();
    if (!window || !monitor || amount <= 0.F)
        return;

    // Bound fresh on every call: a cached handle can outlive the value it
    // points at across config reloads.
    const auto PENABLEBLURGLOBAL = CConfigValue<Config::BOOL>("decoration:blur:enabled");
    if (!*PENABLEBLURGLOBAL)
        return;

    // Pass bounds use logical coordinates, but OpenGL draws physical pixels.
    CBox box = windowBoxOnMonitor(monitor).scale(monitor->m_scale).round();
    if (box.w < 1 || box.h < 1)
        return;

    const int rounding = static_cast<int>(window->rounding());

    // A fully transparent rect: only the blurred contents beneath show
    // through, scaled by the animated amount for the fade-in.
    Render::GL::g_pHyprOpenGL->renderRect(box, CHyprColor{0, 0, 0, 0}, {.round = rounding, .blur = true, .blurA = amount});
}
