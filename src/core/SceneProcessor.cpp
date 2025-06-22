#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>

SceneProcessor::SceneProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {}

void SceneProcessor::Process(const std::string& scene_id) {
    const auto& scene_data = data_.Get("scenes", scene_id);
    Display(scene_data);
    
    auto choices = GetAvailableChoices(scene_data);
    if (choices.empty()) {
        throw std::runtime_error("No valid choices for scene: " + scene_id);
    }

    std::cout << "\nВаши действия:\n";
    for (size_t i = 0; i < choices.size(); ++i) {
        std::cout << i + 1 << ". " << choices[i].text << "\n";
    }

    int choice_index = rpg_utils::Input::GetInt(1, choices.size()) - 1;
    ExecuteChoice(choices[choice_index]);
}

void SceneProcessor::Display(const nlohmann::json& scene) {
    if (!scene.contains("text")) return;

    const auto& text = scene["text"];
    if (text.is_string()) {
        std::cout << "\n" << text.get<std::string>() << "\n";
    } else if (text.is_array()) {
        for (const auto& line : text) {
            std::cout << "\n" << line.get<std::string>() << "\n";
        }
    }
}

std::vector<SceneProcessor::Choice> SceneProcessor::GetAvailableChoices(
    const nlohmann::json& scene) {

    std::vector<Choice> available_choices;
    if (!scene.contains("choices")) return available_choices;

    for (const auto& choice_json : scene["choices"]) {
        // Check condition if present
        if (choice_json.contains("condition")) {
            const auto& condition = choice_json["condition"].get<std::string>();
            if (!rpg_utils::EvaluateCondition(condition, state_.flags)) {
                continue;
            }
        }

        Choice choice;
        choice.text = choice_json["text"].get<std::string>();

        if (choice_json.contains("effects")) {
            choice.effects = choice_json["effects"];
        }

        if (choice_json.contains("check")) {
            choice.check = choice_json["check"];
        }

        // New fields for simplified format
        if (choice_json.contains("next_success")) {
            choice.next_success = choice_json["next_success"].get<std::string>();
        }
        if (choice_json.contains("next_fail")) {
            choice.next_fail = choice_json["next_fail"].get<std::string>();
        }
        if (choice_json.contains("next_scene")) {
            choice.next_scene = choice_json["next_scene"].get<std::string>();
        }

        available_choices.push_back(std::move(choice));
    }

    return available_choices;
}

void SceneProcessor::ProcessTransition(const std::string& target) {
    if (target.rfind("combat_", 0) == 0) {
        // Боевая сцена
        state_.active_type = "combat";
        state_.combat_enemy = target;
        state_.combat_player_health = state_.current_health;
        state_.combat_enemy_health = data_.Get("combat", target)["health"].get<int>();
        state_.player_turn = true;
    }
    else if (target.rfind("ending", 0) == 0) {
        // Концовка
        state_.active_type = "ending";
        state_.active_id = target;
        state_.unlocked_endings.insert(target);
    }
    else if (target == "character_creation") {
        // Создание персонажа
        state_.active_type = "character_creation";
        state_.active_id = target;
    }
    else if (target.rfind("scene", 0) == 0) {
        // Обычная сцена
        state_.active_type = "scene";
        state_.active_id = target;
    }
    else {
        throw std::runtime_error("Unknown transition target: " + target);
    }
}


void SceneProcessor::ExecuteChoice(const Choice& choice) {
    // Применяем немедленные эффекты выбора
    if (!choice.effects.is_null()) {
        rpg_utils::ApplyEffects(choice.effects, state_);
    }

    // Обрабатываем проверку характеристики
    if (!choice.check.is_null()) {
        // Проверяем тип объекта проверки
        if (choice.check.is_string()) {
            // Упрощенная обработка для формата сцены 7
            const std::string stat = choice.check.get<std::string>();
            std::cout << "[DEBUG] Проверка характеристики: " << stat << "\n";

            // Рассчитываем значение характеристики игрока
            const int player_stat = rpg_utils::CalculateStat(stat, state_.stats, data_);

            // Бросаем кубики
            const int roll = rpg_utils::RollDice(3, 6);
            const int result = player_stat - roll;

            // Определяем уровень успеха
            bool success = (result >= 0);
            std::cout << "[DEBUG] Результат: " << (success ? "Успех" : "Провал") << "\n";

            // Определяем следующий переход
            std::string next_target;
            if (success && !choice.next_success.empty()) {
                next_target = choice.next_success;
            }
            else if (!success && !choice.next_fail.empty()) {
                next_target = choice.next_fail;
            }

            if (!next_target.empty()) {
                // Получаем данные исхода
                const auto& outcome_data = data_.Get("checks", next_target);

                // Показываем и обрабатываем результат
                Display(outcome_data);
                rpg_utils::ApplyEffects(outcome_data, state_);

                // Обрабатываем переход
                if (outcome_data.contains("next_scene")) {
                    ProcessTransition(outcome_data["next_scene"].get<std::string>());
                    return;
                }

                // Обрабатываем вложенные проверки
                if (outcome_data.contains("next_check")) {
                    const auto& next_check = outcome_data["next_check"];
                    Choice nested_choice;
                    nested_choice.text = outcome_data["text"].get<std::string>();
                    nested_choice.check = next_check["type"];
                    nested_choice.next_success = next_check["success"];
                    nested_choice.next_fail = next_check["fail"];

                    // Рекурсивно обрабатываем вложенную проверку
                    ExecuteChoice(nested_choice);
                    return;
                }
            }
        }
        else if (choice.check.is_object()) {
            // Обработка для других сцен (расширенный формат)
            // ... существующий код для объектного формата ...
        }
    }

    // Стандартный переход
    if (!choice.next_target.empty()) {
        ProcessTransition(choice.next_target);
    }
    else if (!choice.next_scene.empty()) {
        ProcessTransition(choice.next_scene);
    }
    else {
        // Возврат в текущую сцену, если нет перехода
        ProcessTransition(state_.active_id);
    }
}
