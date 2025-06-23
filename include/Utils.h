// Utils.h
#pragma once
#include <string>
#include <vector>

namespace rpg_utils {
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
    std::string ToLower(const std::string& str);

    enum class RollResultType { kCriticalSuccess, kSuccess, kFail, kCriticalFail };

    struct RollDetails {
        int total_roll;
        RollResultType result;
    };

    RollDetails RollDiceWithModifiers(int base_value, int modifier);
    int CalculateDamage(const std::string& dice_formula);

    class Input {
    public:
        static int GetInt(int min, int max);
        static std::string GetLine();
    };
}