#pragma once

#define WLR_USE_UNSTABLE

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>

#include "BlurPassElement.hpp"

// Transparent overlay covering an unfocused window. It draws nothing of its
// own and only asks the compositor to show the blurred contents beneath,
// with the blur amount animated from clear to full by the tracker.
class CBlurDeco : public IHyprWindowDecoration {
  public:
    explicit CBlurDeco(PHLWINDOW window);
    virtual ~CBlurDeco() = default;

    virtual SDecorationPositioningInfo getPositioningInfo() override;

    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply) override;

    virtual void                       draw(PHLMONITOR monitor, float const& a) override;

    virtual eDecorationType            getDecorationType() override;

    virtual void                       updateWindow(PHLWINDOW window) override;

    virtual void                       damageEntire() override;

    virtual eDecorationLayer           getDecorationLayer() override;

    virtual uint64_t                   getDecorationFlags() override;

    virtual std::string                getDisplayName() override;

    void                               renderPass(PHLMONITOR monitor, float amount);

    // Monitor-local logical coordinates, as required by pass boundingBox().
    CBox windowBoxOnMonitor(PHLMONITOR monitor);

  private:
    PHLWINDOWREF m_window;
    // Last reported amount, so transitions log exactly once.
    double m_lastReportedAmount = 0.0;
};
