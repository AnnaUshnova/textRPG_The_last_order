#include "CharacterCreator.h"
#include <iostream>
#include <vector>
#include <stdexcept>

CharacterCreator::CharacterCreator(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void CharacterCreator::Process() {
    // Инициализация базовых характеристик, если они ещё не установлены
    if (state_.stats.empty()) {
        const auto& char_base = data_.Get("character_base");
        for (const auto& [stat, value] : char_base.items()) {
            if (stat != "points_to_distribute" && stat != "descriptions") {
                state_.stats[stat] = value.get<int>();
            }
        }
        state_.stat_points = char_base["points_to_distribute"].get<int>();
    }

    // Основной цикл распределения очков
    while (state_.stat_points > 0) {
        DisplayStats();
        DistributePoints();
    }

    // После распределения сохраняем состояние
    state_.flags["character_created"] = true;
    std::cout << "\nСоздание персонажа завершено!\n";
}

void CharacterCreator::DisplayStats() {
    const auto& char_base = data_.Get("character_base");
    const auto& descriptions = char_base["descriptions"];

    std::cout << "\nОчки для распределения: " << state_.stat_points << "\n";
    std::cout << "Характеристики:\n";

    // Выводим базовые характеристики
    for (const auto& stat : { "melee", "ranged", "strength", "endurance", "dexterity", "intelligence" }) {
        if (state_.stats.find(stat) != state_.stats.end()) {
            std::cout << "  " << stat << ": " << state_.stats.at(stat);
            if (descriptions.contains(stat)) {
                std::cout << " - " << descriptions[stat].get<std::string>();
            }
            std::cout << "\n";
        }
    }

    // Выводим производные характеристики
    for (const auto& stat : { "health", "willpower" }) {
        if (state_.stats.find(stat) != state_.stats.end()) {
            std::cout << "  " << stat << ": " << state_.stats.at(stat);
            if (descriptions.contains(stat)) {
                std::cout << " - " << descriptions[stat].get<std::string>();
            }
            std::cout << "\n";
        }
    }
}

void CharacterCreator::DistributePoints() {
    // Создаем список доступных для улучшения характеристик
    std::vector<std::string> upgradable_stats = {
        "melee", "ranged", "strength", "endurance", "dexterity", "intelligence"
    };

    // Выводим меню выбора
    std::cout << "\nВыберите характеристику для улучшения:\n";
    for (size_t i = 0; i < upgradable_stats.size(); ++i) {
        std::cout << i + 1 << ". " << upgradable_stats[i] << "\n";
    }
    std::cout << upgradable_stats.size() + 1 << ". Завершить распределение\n";

    // Получаем выбор игрока
    int choice = rpg_utils::Input::GetInt(1, upgradable_stats.size() + 1);

    // Обработка выбора
    if (choice <= static_cast<int>(upgradable_stats.size())) {
        const std::string& selected_stat = upgradable_stats[choice - 1];
        if (ApplyPoint(selected_stat)) {
            std::cout << selected_stat << " увеличен на 1 пункт.\n";
        }
        else {
            std::cout << "Недостаточно очков для улучшения.\n";
        }
    }
    else {
        state_.stat_points = 0; // Завершаем распределение
    }
}

bool CharacterCreator::ApplyPoint(const std::string& stat) {
    if (state_.stat_points <= 0) return false;

    // Увеличиваем выбранную характеристику
    state_.stats[stat]++;
    state_.stat_points--;

    // Пересчитываем производные характеристики
    const auto& char_base = data_.Get("character_base");
    if (char_base.contains("derived_stats")) {
        const auto& derived_stats = char_base["derived_stats"];

        for (const auto& [derived_stat, formula] : derived_stats.items()) {
            try {
                std::string formula_str = formula.get<std::string>();
                auto tokens = rpg_utils::Split(rpg_utils::Trim(formula_str), ' ');

                if (tokens.size() == 3 && tokens[1] == "*") {
                    if (tokens[0] == stat) {
                        state_.stats[derived_stat] = state_.stats[stat] * std::stoi(tokens[2]);
                    }
                }
            }
            catch (...) {
                // Игнорируем ошибки расчета производных характеристик
            }
        }
    }

    return true;
}