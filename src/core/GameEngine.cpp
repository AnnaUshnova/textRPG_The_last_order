#include "GameEngine.h"
#include "Utils.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>

GameEngine::GameEngine(const std::string& data_path) {
    data_.LoadAll(data_path);

    // Загрузка начального состояния
    const auto& initial = data_.Get("game_state");
    state_.active_id = initial["current_scene"].get<std::string>();
    state_.active_type = "scene";

    for (const auto& [key, value] : initial["flags"].items()) {
        state_.flags[key] = value.get<bool>();
    }

    // Загрузка характеристик персонажа
    if (initial.contains("character")) {
        const auto& char_data = initial["character"];
        for (const auto& [stat, value] : char_data.items()) {
            state_.stats[stat] = value.get<int>();
        }
    }

    // Загрузка коллекции концовок
    if (initial.contains("known_endings")) {
        for (const auto& ending : initial["known_endings"]) {
            state_.unlocked_endings.insert(ending.get<std::string>());
        }
    }
}

void GameEngine::Run() {
    while (true) {
        if (state_.active_type == "scene") {
            ProcessScene(state_.active_id);
        }
        else if (state_.active_type == "combat") {
            ProcessCombat(state_.active_id);
        }
        else if (state_.active_type == "ending") {
            ProcessEnding(state_.active_id);
        }
        else if (state_.active_type == "character") {
            ProcessCharacterCreation();
        }
    }
}

void GameEngine::ProcessScene(const std::string& scene_id) {
    const auto& scene = data_.Get("scenes", scene_id);

    // Отображение текста
    for (const auto& line : scene["text"]) {
        std::cout << line.get<std::string>() << "\n";
    }

    // Обработка выборов
    using ChoiceList = std::vector<nlohmann::json>;
    ChoiceList available_choices;

    for (const auto& choice : scene["choices"]) {
        if (!choice.contains("condition") || EvaluateCondition(choice["condition"].get<std::string>())) {
            available_choices.push_back(choice);
        }
    }

    // Отображение выборов
    for (size_t i = 0; i < available_choices.size(); i++) {
        std::cout << i + 1 << ". " << available_choices[i]["text"].get<std::string>() << "\n";
    }

    // Получение выбора игрока
    int choice_index = Input::GetInt(1, static_cast<int>(available_choices.size())) - 1;
    const auto& choice = available_choices[choice_index];

    // Обработка эффектов
    if (choice.contains("effects")) {
        ApplyEffects(choice["effects"]);
    }

    // Обработка проверки
    if (choice.contains("check")) {
        const auto& check_data = choice["check"];
        RollResult result = RollCheck(check_data["stat"].get<std::string>());

        // Определение результата на основе броска
        std::string outcome;
        if (result == RollResult::kCriticalSuccess && check_data.contains("critical_success")) {
            outcome = check_data["critical_success"].get<std::string>();
        }
        else if (result == RollResult::kSuccess && check_data.contains("success")) {
            outcome = check_data["success"].get<std::string>();
        }
        else if (result == RollResult::kFail && check_data.contains("fail")) {
            outcome = check_data["fail"].get<std::string>();
        }
        else if (result == RollResult::kCriticalFail && check_data.contains("critical_fail")) {
            outcome = check_data["critical_fail"].get<std::string>();
        }
        else {
            outcome = check_data["default"].get<std::string>();
        }

        // Переход к следующему элементу
        state_.active_id = outcome;
        if (outcome.rfind("combat_", 0) == 0) state_.active_type = "combat";
        else if (outcome.rfind("ending", 0) == 0) state_.active_type = "ending";
        else state_.active_type = "scene";
    }
    // Прямой переход
    else if (choice.contains("next")) {
        state_.active_id = choice["next"].get<std::string>();
        if (state_.active_id.rfind("combat_", 0) == 0) state_.active_type = "combat";
        else if (state_.active_id.rfind("ending", 0) == 0) state_.active_type = "ending";
        else if (state_.active_id == "character_creation") state_.active_type = "character";
    }
}

void GameEngine::ProcessCombat(const std::string& combat_id) {
    const auto& combat = data_.Get("combat", combat_id);

    // Инициализация боя
    if (state_.combat_enemy_health == 0) {
        state_.combat_enemy = combat["enemy"].get<std::string>();
        state_.combat_enemy_health = combat["health"].get<int>();
        state_.combat_player_health = CalculateStat("health");
        state_.player_turn = true;
    }

    // Ход игрока
    if (state_.player_turn) {
        std::cout << "\n--- ВАШ ХОД ---\n";
        std::cout << "Ваше здоровье: " << state_.combat_player_health << "\n";
        std::cout << "Здоровье " << state_.combat_enemy << ": "
            << state_.combat_enemy_health << "\n\n";

        // Отображение действий
        const auto& actions = combat["player_turn"]["options"];
        for (size_t i = 0; i < actions.size(); i++) {
            std::cout << i + 1 << ". " << actions[i]["type"].get<std::string>() << "\n";
        }

        // Обработка выбора
        int action_index = Input::GetInt(1, static_cast<int>(actions.size())) - 1;
        ResolveCombatAction(actions[action_index]);
    }
    // Ход противника
    else {
        std::cout << "\n--- ХОД ПРОТИВНИКА ---\n";

        // Автоматический выбор действия
        const auto& actions = combat["enemy_turn"]["actions"];
        size_t action_index = rand() % actions.size();
        ResolveCombatAction(actions[action_index]);
    }

    // Проверка условий конца боя
    if (state_.combat_enemy_health <= 0) {
        std::cout << combat["on_win"]["text"].get<std::string>() << "\n";
        ApplyEffects(combat["on_win"]["effects"]);
        state_.active_type = "scene";
        state_.active_id = combat["on_win"]["next"].get<std::string>();
        state_.combat_enemy_health = 0; // Сброс состояния боя
    }
    else if (state_.combat_player_health <= 0) {
        std::cout << combat["on_lose"]["text"].get<std::string>() << "\n";
        state_.active_type = "ending";
        state_.active_id = combat["on_lose"]["next"].get<std::string>();
        state_.combat_enemy_health = 0; // Сброс состояния боя
    }
}

void GameEngine::ProcessEnding(const std::string& ending_id) {
    const auto& ending = data_.Get("endings", ending_id);

    std::cout << "\n=== " << ending["title"].get<std::string>() << " ===\n";
    std::cout << ending["text"].get<std::string>() << "\n";
    std::cout << "Achievement: " << ending["achievement"].get<std::string>() << "\n";

    // Добавление в коллекцию
    state_.unlocked_endings.insert(ending_id);

    // Сохранение прогресса
    nlohmann::json save_data;
    for (const auto& ending_id : state_.unlocked_endings) {
        save_data["known_endings"].push_back(ending_id);
    }

    std::ofstream file("data/save.json");
    if (file) {
        file << save_data.dump(4);
    }
    else {
        std::cerr << "Failed to save game data\n";
    }

    // Показ коллекции
    std::cout << "\n\n--- ВАШИ КОНЦОВКИ ---\n";
    for (const auto& ending_id : state_.unlocked_endings) {
        const auto& ending = data_.Get("endings", ending_id);
        std::cout << "- " << ending["title"].get<std::string>()
            << " [" << ending["achievement"].get<std::string>() << "]\n";
    }

    // Возврат в главное меню или завершение
    state_.active_type = "scene";
    state_.active_id = "main_menu";
}

void GameEngine::ApplyEffects(const nlohmann::json& effects) {
    if (effects.is_null()) return;

    if (effects.contains("set_flags")) {
        for (const auto& [flag, value] : effects["set_flags"].items()) {
            state_.flags[flag] = value.get<bool>();
        }
    }

    if (effects.contains("add_items") && effects["add_items"].is_array()) {
        for (const auto& item : effects["add_items"]) {
            state_.inventory.push_back(item.get<std::string>());
        }
    }

    if (effects.contains("remove_items") && effects["remove_items"].is_array()) {
        for (const auto& item : effects["remove_items"]) {
            auto it = std::find(state_.inventory.begin(), state_.inventory.end(), item.get<std::string>());
            if (it != state_.inventory.end()) {
                state_.inventory.erase(it);
            }
        }
    }

    if (effects.contains("modify_stats")) {
        for (const auto& [stat, value] : effects["modify_stats"].items()) {
            state_.stats[stat] += value.get<int>();
        }
    }
}

bool GameEngine::EvaluateCondition(const std::string& condition) const {
    // Простая реализация для флагов
    if (condition.empty()) return true;

    if (condition[0] == '!') {
        std::string flag = condition.substr(1);
        return !(state_.flags.find(flag) != state_.flags.end() && state_.flags.at(flag));
    }
    return state_.flags.find(condition) != state_.flags.end() && state_.flags.at(condition);
}

GameEngine::RollResult GameEngine::RollCheck(const std::string& stat_name) const {
    int stat_value = CalculateStat(stat_name);
    int roll = Utils::RollDice(2, 6);

    if (roll <= stat_value / 2) return RollResult::kCriticalSuccess;
    if (roll <= stat_value) return RollResult::kSuccess;
    if (roll > stat_value * 2) return RollResult::kCriticalFail;
    return RollResult::kFail;
}

int GameEngine::CalculateStat(const std::string& stat_name) const {
    // Проверка базовых характеристик
    if (state_.stats.find(stat_name) != state_.stats.end()) {
        return state_.stats.at(stat_name);
    }

    // Проверка производных характеристик
    const auto& char_base = data_.Get("character_base");
    if (char_base.contains("derived_stats") &&
        char_base["derived_stats"].contains(stat_name)) {

        const auto& formula = char_base["derived_stats"][stat_name].get<std::string>();
        // Простая реализация формул типа "endurance * 2"
        if (formula.find('*') != std::string::npos) {
            auto parts = Utils::Split(formula, '*');
            if (parts.size() != 2) return 0;

            std::string base_stat = Utils::Trim(parts[0]);
            int multiplier = std::stoi(Utils::Trim(parts[1]));
            return CalculateStat(base_stat) * multiplier;
        }
    }

    return 0; // Стандартное значение
}

void GameEngine::ResolveCombatAction(const nlohmann::json& action) {
    const std::string& action_type = action["type"].get<std::string>();
    RollResult result = RollCheck(action_type);

    // Получение результата действия
    std::string result_key;
    switch (result) {
    case RollResult::kCriticalSuccess: result_key = "critical_success"; break;
    case RollResult::kSuccess: result_key = "success"; break;
    case RollResult::kFail: result_key = "fail"; break;
    case RollResult::kCriticalFail: result_key = "critical_fail"; break;
    }

    // Отображение результата
    if (action["results"].contains(result_key)) {
        std::cout << action["results"][result_key].get<std::string>() << "\n";
    }

    // Применение эффектов
    if (action.contains("effects")) {
        ApplyEffects(action["effects"]);
    }

    // Переключение хода
    state_.player_turn = !state_.player_turn;
}

void GameEngine::ProcessCharacterCreation() {
    // Реализация создания персонажа
    // ...
    state_.active_type = "scene";
    state_.active_id = "scene7";
}