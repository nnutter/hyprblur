#include "BlurPassElement.hpp"

#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include "BlurDeco.hpp"

CBlurPassElement::CBlurPassElement(const CBlurPassElement::SBlurData& data) : m_data(data) {
    ;
}

std::vector<UP<IPassElement>> CBlurPassElement::draw() {
    m_data.deco->renderPass(g_pHyprRenderer->m_renderData.pMonitor.lock(), m_data.amount);
    return {};
}

bool CBlurPassElement::needsLiveBlur() {
    // Bound fresh on every call: a cached handle can outlive the value it
    // points at across config reloads.
    const auto PENABLEBLURGLOBAL = CConfigValue<Config::BOOL>("decoration:blur:enabled");

    return *PENABLEBLURGLOBAL && m_data.amount > 0.F;
}

bool CBlurPassElement::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> CBlurPassElement::boundingBox() {
    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    if (!monitor)
        return std::nullopt;

    // A small margin keeps occlusion culling from clipping the blurred edges.
    return m_data.deco->windowBoxOnMonitor(monitor).expand(10);
}
