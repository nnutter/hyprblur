#pragma once

#include <regex>
#include <string>

namespace HyprBlur {
    // Empty patterns opt no windows in. Nonempty patterns match the whole
    // class or title; regex alternatives can select multiple applications.
    inline bool matchesWindowRule(const std::string& pattern, const std::string& value) {
        return !pattern.empty() && std::regex_match(value, std::regex{pattern});
    }

    inline bool initiallyBlurred(bool blurByDefault, const std::string& classPattern, const std::string& titlePattern, const std::string& appClass, const std::string& title) {
        return blurByDefault || matchesWindowRule(classPattern, appClass) || matchesWindowRule(titlePattern, title);
    }
} // namespace
