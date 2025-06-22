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

CombatProcessor::RollResult CombatProcessor::RollCheck(const std::string& stat_name) const {
    // Для простых проверок врага используем фиксированные пороги
    int target_value = 12; // Стандартная сложность для врага

    auto roll = rpg_utils::RollWithDetails(target_value);
    rpg_utils::PrintRollDetails(stat_name, 0, 0, target_value, roll);

    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess: return RollResult::kCriticalSuccess;
    case rpg_utils::RollResultType::kSuccess: return RollResult::kSuccess;
    case rpg_utils::RollResultType::kFail: return RollResult::kFail;
    case rpg_utils::RollResultType::kCriticalFail: return RollResult::kCriticalFail;
    }
    return RollResult::kFail; // fallback
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

void CombatProcessor::ResolveAction(const nlohmann::json& action) {
    // Получаем данные для броска
    std::string stat = action.value("stat", "dexterity");
    int difficulty = action.value("difficulty", 0);

    // Рассчитываем значение характеристики
    int player_stat = rpg_utils::CalculateStat(stat, state_.stats, data_);
    int target_value = player_stat + difficulty;

    // Выполняем бросок с детализацией
    auto roll = rpg_utils::RollWithDetails(target_value);

    // Получаем отображаемое имя характеристики
    const auto& char_base = data_.Get("character_base");
    const auto& display_names = char_base["display_names"];
    std::string stat_display_name = display_names.value(stat, stat);

    rpg_utils::PrintRollDetails(
        "Боевое действие: " + action["type"].get<std::string>(),
        player_stat,
        difficulty,
        target_value,
        roll
    );

    // Преобразуем в боевой результат
    RollResult result;
    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess:
        result = RollResult::kCriticalSuccess;
        break;
    case rpg_utils::RollResultType::kSuccess:
        result = RollResult::kSuccess;
        break;
    case rpg_utils::RollResultType::kFail:
        result = RollResult::kFail;
        break;
    case rpg_utils::RollResultType::kCriticalFail:
        result = RollResult::kCriticalFail;
        break;
    }

    // Получаем текст результата
    const std::string result_str = RollResultToString(result);
    const std::string& outcome = action["results"][result_str].get<std::string>();

    std::cout << outcome << "\n";

    // Применяем эффекты действия
    switch (result) {
    case RollResult::kCriticalSuccess:
        state_.combat_enemy_health -= 10;
        break;
    case RollResult::kSuccess:
        state_.combat_enemy_health -= 5;
        break;
    case RollResult::kFail:
        // Небольшой урон при провале
        state_.combat_enemy_health -= 1;
        break;
    case RollResult::kCriticalFail:
        // Критический провал - возможны негативные эффекты для игрока
        state_.combat_player_health -= 2;
        break;
    }

    // Обновляем статус
    std::cout << "\n[Здоровье игрока: " << state_.combat_player_health
        << " | Здоровье врага: " << state_.combat_enemy_health << "]\n\n";
}

void CombatProcessor::PlayerTurn(const nlohmann::json& combat) {
    const auto& options = combat["player_turn"]["options"];

    // Проверяем наличие доступных действий
    if (options.empty()) {
        std::cout << "У вас нет доступных действий!\n";
        return;
    }

    std::cout << "\n=== ВАШ ХОД ===\n";
    std::cout << "Выберите действие:\n";
    for (size_t i = 0; i < options.size(); ++i) {
        std::string action_name = options[i]["type"].get<std::string>();
        std::cout << i + 1 << ". " << action_name << "\n";
    }

    int choice = rpg_utils::Input::GetInt(1, options.size()) - 1;
    ResolveAction(options[choice]);
}

void CombatProcessor::EnemyTurn(const nlohmann::json& combat) {
    std::cout << "\n=== ХОД ПРОТИВНИКА ===\n";

    // Выполняем бросок для врага (фиксированная сложность 12)
    auto roll = rpg_utils::RollWithDetails(12);
    rpg_utils::PrintRollDetails("Атака врага", 0, 0, 12, roll);

    // Преобразуем в боевой результат
    RollResult result;
    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess:
        result = RollResult::kCriticalSuccess;
        break;
    case rpg_utils::RollResultType::kSuccess:
        result = RollResult::kSuccess;
        break;
    case rpg_utils::RollResultType::kFail:
        result = RollResult::kFail;
        break;
    case rpg_utils::RollResultType::kCriticalFail:
        result = RollResult::kCriticalFail;
        break;
    }

    const std::string result_str = RollResultToString(result);
    const std::string& outcome = combat["enemy_turn"]["results"][result_str].get<std::string>();

    std::cout << outcome << "\n";

    // Применяем урон на основе результата
    switch (result) {
    case RollResult::kCriticalSuccess:
        state_.combat_player_health -= 8;
        break;
    case RollResult::kSuccess:
        state_.combat_player_health -= 4;
        break;
    case RollResult::kFail:
        state_.combat_player_health -= 1;
        break;
    case RollResult::kCriticalFail:
        // Критический провал противника
        state_.combat_enemy_health -= 3;
        std::cout << "Противник наносит урон себе!\n";
        break;
    }

    // Обновляем статус
    std::cout << "\n[Здоровье игрока: " << state_.combat_player_health
        << " | Здоровье врага: " << state_.combat_enemy_health << "]\n\n";
}