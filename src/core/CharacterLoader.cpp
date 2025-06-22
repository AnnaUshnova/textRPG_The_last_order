#include "CharacterLoader.h"
#include "Utils.h"
#include <stdexcept>

void CharacterLoader::LoadFromJson(GameState& state, const nlohmann::json& data) {
    // Загрузка базовых характеристик
    const std::vector<std::string> base_stats = {
        "melee", "ranged", "strength", "endurance", "dexterity", "intelligence"
    };

    for (const auto& stat : base_stats) {
        if (data.contains(stat)) {
            state.stats[stat] = data[stat].get<int>();
        }
    }

    // Загрузка производных характеристик (health, willpower)
    if (data.contains("derived_stats") && data["derived_stats"].is_object()) {
        const auto& derived_stats = data["derived_stats"];
        for (const auto& [derived_stat, formula] : derived_stats.items()) {
            try {
                std::string formula_str = formula.get<std::string>();
                auto tokens = rpg_utils::Split(rpg_utils::Trim(formula_str), ' ');

                if (tokens.size() == 3 && tokens[1] == "*") {
                    std::string base_stat = tokens[0];
                    int multiplier = std::stoi(tokens[2]);

                    if (state.stats.find(base_stat) != state.stats.end()) {
                        state.stats[derived_stat] = state.stats[base_stat] * multiplier;
                    }
                }
            }
            catch (...) {
                // Обработка ошибок расчета
            }
        }
    }

    // Загрузка сопротивлений
    if (data.contains("resistance") && data["resistance"].is_object()) {
        for (const auto& [res_type, value] : data["resistance"].items()) {
            state.resistances[res_type] = value.get<int>();
        }
    }

    // Загрузка очков улучшения
    if (data.contains("points_to_distribute")) {
        state.stat_points = data["points_to_distribute"].get<int>();
    }

    // Инициализация текущего здоровья
    if (state.stats.find("health") != state.stats.end()) {
        state.current_health = state.stats["health"];
    }
}