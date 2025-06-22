#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>

SceneProcessor::SceneProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {}

void SceneProcessor::Process(const std::string& scene_id) {
    const auto& scene_data = data_.Get("scenes", scene_id);

    // Выводим текст только при первом входе в сцену
    if (state_.last_scene != scene_id) {
        Display(scene_data);
        state_.last_scene = scene_id;  // Запоминаем текущую сцену
    }

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
    }
    else if (text.is_array()) {
        for (const auto& line : text) {
            std::cout << "\n" << line.get<std::string>() << "\n";
        }
    }

    // Показываем доступные выборы с указанием сложности
    if (scene.contains("choices")) {
        std::cout << "\nДоступные действия:\n";
        for (const auto& choice_json : scene["choices"]) {
            if (choice_json.contains("condition")) {
                const auto& condition = choice_json["condition"].get<std::string>();
                if (!rpg_utils::EvaluateCondition(condition, state_.flags)) {
                    continue;
                }
            }

            std::string text = choice_json["text"].get<std::string>();

            // Добавляем информацию о сложности
            if (choice_json.contains("check")) {
                int difficulty = 0;
                if (choice_json["check"].is_object() &&
                    choice_json["check"].contains("difficulty")) {
                    difficulty = choice_json["check"]["difficulty"].get<int>();
                }

                if (difficulty != 0) {
                    text += " [Сложность: ";
                    if (difficulty > 0) {
                        text += "+";
                    }
                    text += std::to_string(difficulty) + "]";
                }
                else {
                    text += " [Стандартная сложность]";
                }
            }

            std::cout << " - " << text << "\n";
        }
    }
}


std::vector<SceneProcessor::Choice> SceneProcessor::GetAvailableChoices(
    const nlohmann::json& scene) {

    std::vector<Choice> available_choices;
    if (!scene.contains("choices")) return available_choices;

    for (const auto& choice_json : scene["choices"]) {
        // Проверка условия
        if (choice_json.contains("condition")) {
            const auto& condition = choice_json["condition"].get<std::string>();
            if (!rpg_utils::EvaluateCondition(condition, state_.flags)) {
                continue;
            }
        }

        // Проверка, что выбор еще не делался
        if (choice_json.contains("flag_to_set")) {
            const std::string flag = choice_json["flag_to_set"].get<std::string>();
            if (state_.flags.find(flag) != state_.flags.end() && state_.flags.at(flag)) {
                continue; // Пропускаем уже сделанный выбор
            }
        }

        Choice choice;
        choice.text = choice_json["text"].get<std::string>();
        
        // ID выбора
        if (choice_json.contains("id")) {
            choice.id = choice_json["id"].get<std::string>();
        }
        
        // Флаг для установки
        if (choice_json.contains("flag_to_set")) {
            choice.flag_to_set = choice_json["flag_to_set"].get<std::string>();
        }
        
        // Проверка характеристики
        if (choice_json.contains("check")) {
            // Поддержка старого формата (строка) и нового (объект)
            if (choice_json["check"].is_string()) {
                choice.check = {
                    {"type", choice_json["check"].get<std::string>()},
                    {"difficulty", 0} // Сложность по умолчанию
                };
            } else {
                choice.check = choice_json["check"];
                // Установка сложности по умолчанию
                if (!choice.check.contains("difficulty")) {
                    choice.check["difficulty"] = 0;
                }
            }
        }
        
        // Результаты проверок
        if (choice_json.contains("next_success")) {
            choice.next_success = choice_json["next_success"].get<std::string>();
        }
        if (choice_json.contains("next_fail")) {
            choice.next_fail = choice_json["next_fail"].get<std::string>();
        }
        if (choice_json.contains("critical_success")) {
            choice.critical_success = choice_json["critical_success"].get<std::string>();
        }
        if (choice_json.contains("critical_fail")) {
            choice.critical_fail = choice_json["critical_fail"].get<std::string>();
        }
        
        // Стандартный переход
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
    // Устанавливаем флаг выбора, если он задан
    if (!choice.flag_to_set.empty()) {
        state_.flags[choice.flag_to_set] = true;
    }

    // Применяем немедленные эффекты выбора
    if (!choice.effects.is_null()) {
        rpg_utils::ApplyEffects(choice.effects, state_);
    }

    // Обрабатываем проверку характеристики, если она есть
    if (!choice.check.is_null()) {
        // Извлекаем тип характеристики и сложность
        std::string stat;
        int difficulty = 0;

        // Обработка разных форматов проверки
        if (choice.check.is_string()) {
            stat = choice.check.get<std::string>();
        }
        else if (choice.check.is_object()) {
            stat = choice.check["type"].get<std::string>();
            if (choice.check.contains("difficulty")) {
                difficulty = choice.check["difficulty"].get<int>();
            }
        }
        else {
            throw std::runtime_error("Неподдерживаемый формат проверки");
        }

        // Получаем отображаемое имя характеристики
        const auto& char_base = data_.Get("character_base");
        const auto& display_names = char_base["display_names"];
        std::string stat_display_name = display_names.value(stat, stat);

        // Рассчитываем значение характеристики
        int player_stat = rpg_utils::CalculateStat(stat, state_.stats, data_);
        int target_value = player_stat + difficulty;

        // Выполняем бросок с детализацией
        auto roll = rpg_utils::RollWithDetails(target_value);
        rpg_utils::PrintRollDetails(
            stat_display_name + " проверка",
            player_stat,
            difficulty,
            target_value,
            roll
        );

        // Определяем ID исхода на основе уровня успеха
        std::string outcome_id;
        const std::string& result_key = roll.result_str;

        if (result_key == "critical_success" && !choice.critical_success.empty()) {
            outcome_id = choice.critical_success;
        }
        else if (result_key == "success" && !choice.next_success.empty()) {
            outcome_id = choice.next_success;
        }
        else if (result_key == "fail" && !choice.next_fail.empty()) {
            outcome_id = choice.next_fail;
        }
        else if (result_key == "critical_fail" && !choice.critical_fail.empty()) {
            outcome_id = choice.critical_fail;
        }
        else if (result_key == "success" || result_key == "critical_success") {
            outcome_id = choice.next_success;
        }
        else {
            outcome_id = choice.next_fail;
        }

        // Получаем данные исхода
        const auto& outcome_data = data_.Get("checks", outcome_id);

        // Показываем и обрабатываем результат
        Display(outcome_data);
        rpg_utils::ApplyEffects(outcome_data, state_);

        // Обрабатываем переход из исхода
        if (outcome_data.contains("next_scene")) {
            ProcessTransition(outcome_data["next_scene"].get<std::string>());
            return;
        }

        // Обрабатываем вложенные выборы
        if (outcome_data.contains("choices")) {
            const auto& choices_json = outcome_data["choices"];
            std::vector<Choice> outcome_choices;

            for (const auto& c : choices_json) {
                Choice oc;
                oc.text = c["text"].get<std::string>();

                // Проверка характеристики
                if (c.contains("check")) {
                    if (c["check"].is_string()) {
                        oc.check = c["check"];
                    }
                    else if (c["check"].is_object()) {
                        oc.check = c["check"];
                    }
                }

                // Результаты проверок
                if (c.contains("next_success")) oc.next_success = c["next_success"].get<std::string>();
                if (c.contains("next_fail")) oc.next_fail = c["next_fail"].get<std::string>();
                if (c.contains("critical_success")) oc.critical_success = c["critical_success"].get<std::string>();
                if (c.contains("critical_fail")) oc.critical_fail = c["critical_fail"].get<std::string>();

                // Прямой переход
                if (c.contains("next_scene")) oc.next_scene = c["next_scene"].get<std::string>();

                outcome_choices.push_back(oc);
            }

            // Показываем варианты выбора
            std::cout << "\n";
            for (size_t i = 0; i < outcome_choices.size(); ++i) {
                std::cout << i + 1 << ". " << outcome_choices[i].text << "\n";
            }

            // Получаем выбор игрока
            int choice_index = rpg_utils::Input::GetInt(1, outcome_choices.size()) - 1;

            // Рекурсивно обрабатываем выбранный вариант
            ExecuteChoice(outcome_choices[choice_index]);
            return;
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
        ProcessTransition(state_.active_id);
    }
}