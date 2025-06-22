#ifndef RPG_UTILS_H_
#define RPG_UTILS_H_

#include "DataManager.h"
#include "GameState.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

namespace rpg_utils {

    // String utilities
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
    std::string ToLower(const std::string& str);

    // Game utilities
    int RollDice(int count, int sides);
    bool EvaluateCondition(const std::string& condition,
        const std::unordered_map<std::string, bool>& flags);
    void ApplyEffects(const nlohmann::json& effects, GameState& state);
    int CalculateStat(const std::string& stat_name,
        const std::unordered_map<std::string, int>& stats,
        const DataManager& data);

    // Input handling
    class Input {
    public:
        static int GetInt(int min, int max);
        static std::string GetLine();
    };

    enum class RollResultType {
        kCriticalSuccess,
        kSuccess,
        kFail,
        kCriticalFail
    };

    struct RollDetails {
        int total_roll;
        RollResultType result;
        std::string result_str;
    };

    RollDetails RollWithDetails(int target_value);
    void PrintRollDetails(const std::string& context, int base_value, int modifier, int target_value, const RollDetails& roll);


}  // namespace rpg_utils

#endif  // RPG_UTILS_H_