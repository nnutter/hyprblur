#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>

class CBlurDeco;

// Draws the blurred window contents. It only ever exists while the blur
// amount is above zero, so any presence here means live blur is needed.
class CBlurPassElement : public IPassElement {
  public:
    struct SBlurData {
        CBlurDeco* deco   = nullptr;
        float      amount = 0.F;
    };

    explicit CBlurPassElement(const SBlurData& data);
    virtual ~CBlurPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw() override;
    virtual bool                          needsLiveBlur() override;
    virtual bool                          needsPrecomputeBlur() override;
    virtual std::optional<CBox>           boundingBox() override;

    virtual const char*                   passName() override {
        return "CBlurPassElement";
    }

    virtual ePassElementType type() override {
        return EK_CUSTOM;
    }

  private:
    SBlurData m_data;
};
