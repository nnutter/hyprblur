#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <chrono>

#include "fade.hpp"

using namespace std::chrono_literals;
using Catch::Matchers::WithinAbs;
using HyprBlur::CFadeState;
using HyprBlur::SFadeSettings;

namespace {
    constexpr double TOLERANCE = 1e-9;

    SFadeSettings    settings() {
        SFadeSettings s;
        s.focusLostDelay       = 300ms;
        s.fadeRatePercentPerMs = 0.1;
        s.strength             = 0.8;
        return s;
    }
} // namespace

TEST_CASE("default settings reach 90 percent blur after 1200 milliseconds", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s;
    const auto          lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    CHECK_THAT(fade.amount(lost + 1190ms, s), WithinAbs(0.89, TOLERANCE));
    CHECK(fade.animating(lost + 1190ms, s));
    CHECK_THAT(fade.amount(lost + 1200ms, s), WithinAbs(0.90, TOLERANCE));
    CHECK_FALSE(fade.animating(lost + 1200ms, s));
}

TEST_CASE("a focused window has no blur", "[fade]") {
    const CFadeState    fade;
    const SFadeSettings s = settings();

    CHECK_THAT(fade.amount(CFadeState::CTimePoint{}, s), WithinAbs(0.0, TOLERANCE));
    CHECK_FALSE(fade.animating(CFadeState::CTimePoint{}, s));
}

TEST_CASE("blur stays clear until the focus-lost delay passes", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s    = settings();
    const auto          lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    CHECK_THAT(fade.amount(lost + 299ms, s), WithinAbs(0.0, TOLERANCE));
    CHECK(fade.animating(lost + 299ms, s));
}

TEST_CASE("blur rises linearly from the delay to the end of the fade", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s    = settings();
    const auto          lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    // 0.1 percentage points/ms adds 40% blur in 400ms, independent of target.
    CHECK_THAT(fade.amount(lost + 300ms + 400ms, s), WithinAbs(0.4, TOLERANCE));
    CHECK(fade.animating(lost + 300ms + 400ms, s));

    SFadeSettings lowerTarget = s;
    lowerTarget.strength      = 0.5;
    CHECK_THAT(fade.amount(lost + 300ms + 400ms, lowerTarget), WithinAbs(0.4, TOLERANCE));
}

TEST_CASE("blur holds at full strength once the fade completes", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s    = settings();
    const auto          lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    CHECK_THAT(fade.amount(lost + 300ms + 800ms, s), WithinAbs(0.8, TOLERANCE));
    CHECK_FALSE(fade.animating(lost + 300ms + 800ms, s));
    CHECK_THAT(fade.amount(lost + 300ms + 5000ms, s), WithinAbs(0.8, TOLERANCE));
    CHECK_FALSE(fade.animating(lost + 300ms + 5000ms, s));
}

TEST_CASE("regaining focus clears the blur at once, even mid-fade", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s    = settings();
    const auto          lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);
    REQUIRE_THAT(fade.amount(lost + 300ms + 400ms, s), WithinAbs(0.4, TOLERANCE));

    fade.onFocusGained();

    CHECK_THAT(fade.amount(lost + 300ms + 400ms, s), WithinAbs(0.0, TOLERANCE));
    CHECK_THAT(fade.amount(lost + 30000ms, s), WithinAbs(0.0, TOLERANCE));
    CHECK_FALSE(fade.animating(lost + 30000ms, s));
}

TEST_CASE("losing focus again restarts the delay", "[fade]") {
    CFadeState          fade;
    const SFadeSettings s     = settings();
    const auto          first = CFadeState::CTimePoint{};
    fade.onFocusLost(first);
    fade.onFocusGained();

    const auto second = first + 10000ms;
    fade.onFocusLost(second);

    CHECK_THAT(fade.amount(second + 299ms, s), WithinAbs(0.0, TOLERANCE));
    CHECK_THAT(fade.amount(second + 300ms + 800ms, s), WithinAbs(0.8, TOLERANCE));
}

TEST_CASE("a faster fade rate reaches the target sooner", "[fade]") {
    CFadeState    fade;
    SFadeSettings s        = settings();
    s.fadeRatePercentPerMs = 0.2;
    const auto lost        = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    CHECK_THAT(fade.amount(lost + 299ms, s), WithinAbs(0.0, TOLERANCE));
    CHECK_THAT(fade.amount(lost + 300ms + 200ms, s), WithinAbs(0.4, TOLERANCE));
    CHECK(fade.animating(lost + 300ms + 200ms, s));
    CHECK_THAT(fade.amount(lost + 300ms + 400ms, s), WithinAbs(0.8, TOLERANCE));
    CHECK_FALSE(fade.animating(lost + 300ms + 400ms, s));
}

TEST_CASE("strength is clamped to the clear-to-full range", "[fade]") {
    CFadeState fade;
    const auto lost = CFadeState::CTimePoint{};
    fade.onFocusLost(lost);

    SFadeSettings tooStrong = settings();
    tooStrong.strength      = 2.0;
    CHECK_THAT(fade.amount(lost + 300ms + 1000ms, tooStrong), WithinAbs(1.0, TOLERANCE));

    SFadeSettings negative = settings();
    negative.strength      = -1.0;
    CHECK_THAT(fade.amount(lost + 300ms + 1000ms, negative), WithinAbs(0.0, TOLERANCE));
}
