#include <catch2/catch_test_macros.hpp>

#include "window_rules.hpp"

TEST_CASE("window rules select the initial blur state", "[rules]") {
    struct SCase {
        bool        blurByDefault;
        const char* classPattern;
        const char* titlePattern;
        const char* appClass;
        const char* title;
        bool        expected;
    };

    // These cases protect the user-facing matching contract: defaults,
    // independent class/title matches, full matching, and alternatives.
    const SCase cases[] = {
        {false, "", "", "Signal", "Signal", false},
        {false, "", "", "", "", false},
        {true, "^Signal$", "", "terminal", "Shell", true},
        {false, "^Signal$", "", "Signal", "Alice", true},
        {false, "^Signal$", "", "signal", "Signal", false},
        {false, "Signal", "", "Signal Desktop", "Signal", false},
        {false, "^(Signal|signal)$", "", "signal", "Bob", true},
        {false, "^Signal$", ".*Private.*", "terminal", "Private notes", true},
        {false, "^Signal$", ".*Private.*", "Signal", "Alice", true},
        {false, "^Signal$", ".*Private.*", "terminal", "Shell", false},
    };

    for (const auto& test : cases) {
        CAPTURE(test.blurByDefault, test.classPattern, test.titlePattern, test.appClass, test.title);
        CHECK(HyprBlur::initiallyBlurred(test.blurByDefault, test.classPattern, test.titlePattern, test.appClass, test.title) == test.expected);
    }
}

TEST_CASE("invalid window rules report regex errors", "[rules]") {
    CHECK_THROWS_AS(HyprBlur::initiallyBlurred(false, "[", "", "Signal", "Alice"), std::regex_error);
    CHECK_THROWS_AS(HyprBlur::initiallyBlurred(false, "", "[", "Signal", "Alice"), std::regex_error);
}
