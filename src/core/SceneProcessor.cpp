#include "SceneProcessor.h"
#include "Utils.h"
#include "EndingProcessor.h" // Добавляем заголовочный файл
#include <iostream>

using json = nlohmann::json;
using namespace rpg_utils;

SceneProcessor::SceneProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state), ending_processor_(data, state) {
}

void SceneProcessor::Process(const std::string& scene_id) {
    if (!data_.Get("scenes").contains(scene_id)) {
        std::cerr << "ERROR: Scene '" << scene_id << "' not found\n";
        state_.current_scene = "main_menu";
        return;
    }


    const auto& scene = data_.Get("scenes", scene_id);

    // Обработка автоматических действий
    if (scene.contains("effects") && scene["effects"].contains("auto_action")) {
        std::string action = scene["effects"]["auto_action"].get<std::string>();

        if (action == "show_endings") {
            ending_processor_.ShowEndingCollection();
            return;
        }
    }

    // 1. Отображение текста сцены
    DisplayScene(scene);

    // 2. Применение эффектов сцены
    if (scene.contains("effects")) {
        ApplyGameEffects(scene["effects"], state_);
    }

    // 3. Обработка автоматических проверок
    if (scene.contains("auto_check")) {
        ProcessAutoCheck(scene["auto_check"]);
        return;
    }

    // 4. Обработка выборов игрока
    if (scene.contains("choices")) {
        auto choices = GetAvailableChoices(scene["choices"]);
        if (!choices.empty()) {
            // Отображение доступных выборов
            std::cout << "\nВарианты действий:\n";
            for (size_t i = 0; i < choices.size(); ++i) {
                std::cout << i + 1 << ". " << choices[i].text << "\n";
            }

            // Получение выбора игрока
            int choice_index = Input::GetInt(1, choices.size()) - 1;
            ProcessPlayerChoice(choices[choice_index]);
            return;
        }
    }

    // 5. Переход по умолчанию
    if (scene.contains("next_scene")) {
        state_.current_scene = scene["next_scene"].get<std::string>();
    }
    else {
        state_.current_scene = "main_menu"; // Фолбэк
    }
}

void SceneProcessor::DisplayScene(const json& scene) {
    if (scene.contains("text")) {
        for (const auto& line : scene["text"]) {
            std::cout << line.get<std::string>() << "\n\n";
        }
    }
}


void SceneProcessor::ProcessAutoCheck(const json& check_data) {
    // Проверяем наличие необходимых полей
    if (!check_data.contains("type")) {
        std::cerr << "ERROR: Auto check missing 'type'\n";
        return;
    }

    const std::string& stat_name = check_data["type"].get<std::string>();

    // Проверяем существование характеристики
    if (state_.stats.find(stat_name) == state_.stats.end()) {
        std::cerr << "ERROR: Stat '" << stat_name << "' not found\n";
        return;
    }

    int difficulty = check_data.value("difficulty", 0);
    int stat_value = state_.stats.at(stat_name);

    // Остальной код без изменений...
}

void SceneProcessor::ProcessPlayerChoice(const SceneChoice& choice) {
    // Обработка специальных действий
    if (choice.next_target == "quit_game") {
        state_.quit_game = true;
        return;
    }
    // Применение эффектов выбора
    if (!choice.effects.empty()) {
        ApplyGameEffects(choice.effects, state_);
    }

    // Установка флага
    if (!choice.id.empty()) {
        state_.flags[choice.id + "_chosen"] = true;
    }

    // Обработка проверки, если есть
    if (!choice.check.empty()) {
        const std::string& stat_name = choice.check["type"].get<std::string>();
        int difficulty = choice.check.value("difficulty", 0);
        int stat_value = state_.stats.at(stat_name);

        auto roll_result = RollDiceWithModifiers(stat_value, difficulty);
        std::cout << "\nПроверка " << stat_name << ": "
            << roll_result.total_roll << " ("
            << roll_result.result_str << ")\n";

        state_.current_scene = DetermineNextScene(choice.check, roll_result.total_roll);
    }
    else {
        state_.current_scene = choice.next_target;
    }
}

std::string SceneProcessor::DetermineNextScene(const nlohmann::json& check_data, int roll_result) {
    // Проверяем, есть ли явное определение для результата броска
    if (check_data.contains("results")) {
        for (const auto& result : check_data["results"]) {
            // Проверяем условие для результата
            if (result["condition"].get<std::string>() == "success" &&
                roll_result <= result["threshold"]) {
                return result["next_scene"].get<std::string>();
            }
            else if (result["condition"].get<std::string>() == "fail" &&
                roll_result > result["threshold"]) {
                return result["next_scene"].get<std::string>();
            }
        }
    }

    // Стандартная логика на основе типа результата
    auto roll_details = RollDiceWithModifiers(0, 0); // Получаем тип результата
    RollResultType result_type = roll_details.result;

    if (check_data.contains("on_critical_success") &&
        result_type == RollResultType::kCriticalSuccess) {
        return check_data["on_critical_success"].get<std::string>();
    }
    else if (check_data.contains("on_success") &&
        (result_type == RollResultType::kSuccess ||
            result_type == RollResultType::kCriticalSuccess)) {
        return check_data["on_success"].get<std::string>();
    }
    else if (check_data.contains("on_fail") &&
        (result_type == RollResultType::kFail ||
            result_type == RollResultType::kCriticalFail)) {
        return check_data["on_fail"].get<std::string>();
    }
    else if (check_data.contains("on_critical_fail") &&
        result_type == RollResultType::kCriticalFail) {
        return check_data["on_critical_fail"].get<std::string>();
    }

    // Фолбэк на случай отсутствия настроек
    return check_data.value("default_scene", "main_menu");
}


std::vector<SceneProcessor::SceneChoice> SceneProcessor::GetAvailableChoices(const json& choices) {
    std::vector<SceneChoice> available;

    try {
        for (const auto& choice : choices) {
            // Проверка условий отображения выбора
            if (choice.contains("condition")) {
                std::string condition = choice["condition"].get<std::string>();
                if (!EvaluateCondition(condition, state_)) {
                    continue;
                }
            }

            SceneChoice sc;
            sc.id = choice.value("id", "");
            sc.text = choice["text"].get<std::string>();

            if (choice.contains("effects")) {
                sc.effects = choice["effects"];
            }
            if (choice.contains("check")) {
                sc.check = choice["check"];
            }
            if (choice.contains("next_target")) {
                sc.next_target = choice["next_target"].get<std::string>();
            }

            available.push_back(sc);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR in GetAvailableChoices: " << e.what() << "\n";
    }

    return available;
}