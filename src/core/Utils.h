#ifndef RPG_UTILS_H_
#define RPG_UTILS_H_

#include "GameState.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace rpg_utils {

    // String utilities
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Trim(const std::string& str);
    std::string ToLower(const std::string& str);

    // Game utilities
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

    // Централизованный бросок кубиков с модификаторами
    RollDetails RollDiceWithModifiers(
        int base_value,
        int modifier,
        int critical_threshold = 5
    );

    // Проверка условий с учетом флагов и инвентаря
    bool EvaluateCondition(const std::string& condition, const GameState& state);


    // Применение эффектов к состоянию игры
    void ApplyGameEffects(const nlohmann::json& effects, GameState& state);

    // Расчет характеристик
    void CalculateDerivedStats(GameState& state, const nlohmann::json& character_base);

    // Вычисление урона
    int CalculateDamage(const std::string& dice_formula);

    // Централизованный бросок
    RollDetails CalculateRoll(
        const std::string& stat_name,
        int difficulty,
        const GameState& state,
        const std::string& context = ""
    ); 
    
    RollDetails ResolveAttack(
            const nlohmann::json& attack,
            const std::unordered_map<std::string, int>& attacker_stats
        );

    // Input handling
    class Input {
    public:
        static int GetInt(int min, int max);
        static std::string GetLine();    
      
    };

}  // namespace rpg_utils

#endif  // RPG_UTILS_H_