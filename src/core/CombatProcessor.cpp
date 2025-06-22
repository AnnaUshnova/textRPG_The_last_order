#include "CombatProcessor.h"
#include <iostream>
#include <stdexcept>

// Объявляем вспомогательную функцию перед использованием
static std::string RollResultToString(CombatProcessor::RollResult result);

CombatProcessor::CombatProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void CombatProcessor::Process(const std::string& combat_id) {
    const auto& combat_data = data_.Get("combat", combat_id);
    InitializeCombat(combat_data);

    // Основной цикл боя
    while (state_.combat_player_health > 0 && state_.combat_enemy_health > 0) {
        if (state_.player_turn) {
            PlayerTurn(combat_data);
        }
        else {
            EnemyTurn(combat_data);
        }

        // Переключение хода
        state_.player_turn = !state_.player_turn;
    }

    // Обработка результатов боя
    if (state_.combat_player_health <= 0) {
        state_.current_health = 0;  // Обновляем основное здоровье
        // Поражение
        const std::string ending_id = combat_data["on_lose"].get<std::string>();
        state_.active_type = "ending";
        state_.active_id = ending_id;
        state_.unlocked_endings.insert(ending_id);
    }
    else {
        state_.current_health = state_.combat_player_health;  // Сохраняем здоровье после боя

        // Победа
        if (combat_data.contains("on_win")) {
            const auto& win_effects = combat_data["on_win"];

            // Исправление: явно указываем тип для items()
            if (win_effects.contains("set_flags")) {
                for (auto it = win_effects["set_flags"].begin(); it != win_effects["set_flags"].end(); ++it) {
                    const std::string flag = it.key();
                    state_.flags[flag] = it.value().get<bool>();
                }
            }

            if (win_effects.contains("next_scene")) {
                state_.active_type = "scene";
                state_.active_id = win_effects["next_scene"].get<std::string>();
            }
        }
    }
}

void CombatProcessor::InitializeCombat(const nlohmann::json& combat) {
    // Берем здоровье из текущего состояния
    state_.combat_player_health = state_.current_health;

    // Остальной код без изменений
    std::cout << "\nБОЙ: " << combat["enemy"].get<std::string>() << "\n";
    std::cout << "Ваше здоровье: " << state_.combat_player_health << "/"
        << state_.stats.at("health") << "\n";
    std::cout << "Здоровье противника: " << state_.combat_enemy_health << "\n\n";
}

void CombatProcessor::PlayerTurn(const nlohmann::json& combat) {
    const auto& options = combat["player_turn"]["options"];

    std::cout << "Ваш ход. Выберите действие:\n";
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << i + 1 << ". " << options[i]["type"].get<std::string>() << "\n";
    }

    int choice = rpg_utils::Input::GetInt(1, options.size()) - 1;
    ResolveAction(options[choice]);
}

void CombatProcessor::EnemyTurn(const nlohmann::json& combat) {
    std::cout << "\nХод противника...\n";

    // Враги всегда используют стандартную атаку
    RollResult result = RollCheck("enemy_attack");
    const std::string result_str = RollResultToString(result);

    // Исправление: используем at() вместо operator[] для избежания неоднозначности
    const std::string outcome = combat["enemy_turn"]["results"].at(result_str).get<std::string>();

    std::cout << outcome << "\n\n";

    // Применяем урон на основе результата
    switch (result) {
    case RollResult::kCriticalSuccess:
        state_.combat_player_health -= 10;
        break;
    case RollResult::kSuccess:
        state_.combat_player_health -= 5;
        break;
    case RollResult::kFail:
        state_.combat_player_health -= 2;
        break;
    case RollResult::kCriticalFail:
        // Критический провал противника - он наносит урон себе
        state_.combat_enemy_health -= 5;
        break;
    }

    // Обновляем статус
    std::cout << "Ваше здоровье: " << state_.combat_player_health << "\n";
    std::cout << "Здоровье противника: " << state_.combat_enemy_health << "\n\n";
}

void CombatProcessor::ResolveAction(const nlohmann::json& action) {
    RollResult result = RollCheck("attack");  // Стандартная проверка для действий
    const std::string result_str = RollResultToString(result);

    // Получаем текст результата
    const std::string outcome_text = action["results"].at(result_str).get<std::string>();
    std::cout << "\n" << outcome_text << "\n";

    // Применяем эффекты на основе результата
    switch (result) {
    case RollResult::kCriticalSuccess:
        state_.combat_enemy_health -= 15;
        break;
    case RollResult::kSuccess:
        state_.combat_enemy_health -= 10;
        break;
    case RollResult::kFail:
        state_.combat_enemy_health -= 5;
        break;
    case RollResult::kCriticalFail:
        // Критический провал - игрок наносит урон себе
        state_.combat_player_health -= 5;
        break;
    }

    // Специальная обработка для определенных типов действий
    const std::string action_type = action["type"].get<std::string>();
    if (action_type == "medkit") {
        if (result == RollResult::kSuccess || result == RollResult::kCriticalSuccess) {
            // Лечение при успешном использовании аптечки
            int heal_amount = rpg_utils::RollDice(2, 6) + 5;  // 2d6+5
            state_.combat_player_health = std::min(
                state_.stats.at("health"),
                state_.combat_player_health + heal_amount
            );
            std::cout << "Восстановлено " << heal_amount << " здоровья.\n";
        }
    }

    // Обновляем статус
    std::cout << "Ваше здоровье: " << state_.combat_player_health << "\n";
    std::cout << "Здоровье противника: " << state_.combat_enemy_health << "\n\n";
}

CombatProcessor::RollResult CombatProcessor::RollCheck(const std::string& stat_name) const {
    int stat_value = 10;  // Базовое значение для атак

    // Для игрока используем реальные характеристики
    if (stat_name == "attack" && state_.stats.find("melee") != state_.stats.end()) {
        stat_value = state_.stats.at("melee");
    }

    int roll = rpg_utils::RollDice(3, 6);  // 3d6
    int margin = stat_value - roll;

    if (margin >= 5) return RollResult::kCriticalSuccess;
    if (margin >= 0) return RollResult::kSuccess;
    if (margin >= -5) return RollResult::kFail;
    return RollResult::kCriticalFail;
}

void CombatProcessor::ApplyCombatEffects(const nlohmann::json& effects) {
    if (effects.is_null()) return;

    // Обработка эффектов боя
    if (effects.contains("damage")) {
        state_.combat_enemy_health -= effects["damage"].get<int>();
    }

    if (effects.contains("heal")) {
        state_.combat_player_health += effects["heal"].get<int>();
        // Не превышаем максимальное здоровье
        state_.combat_player_health = std::min(
            state_.combat_player_health,
            state_.stats.at("health")
        );
    }
}

// Реализация вспомогательной функции для преобразования RollResult в строку
static std::string RollResultToString(CombatProcessor::RollResult result) {
    switch (result) {
    case CombatProcessor::RollResult::kCriticalSuccess: return "critical_success";
    case CombatProcessor::RollResult::kSuccess: return "success";
    case CombatProcessor::RollResult::kFail: return "fail";
    case CombatProcessor::RollResult::kCriticalFail: return "critical_fail";
    default: return "unknown";
    }
}