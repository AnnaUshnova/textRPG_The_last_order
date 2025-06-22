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

        // Determine next target
        if (choice_json.contains("next_scene")) {
            choice.next_target = choice_json["next_scene"].get<std::string>();
        } else if (choice_json.contains("next_success")) {
            choice.next_target = choice_json["next_success"].get<std::string>();
        } else if (choice_json.contains("next_fail")) {
            choice.next_target = choice_json["next_fail"].get<std::string>();
        } else if (choice_json.contains("critical_success")) {
            choice.next_target = choice_json["critical_success"].get<std::string>();
        } else if (choice_json.contains("critical_fail")) {
            choice.next_target = choice_json["critical_fail"].get<std::string>();
        }

        available_choices.push_back(std::move(choice));
    }

    return available_choices;
}

void SceneProcessor::ExecuteChoice(const Choice& choice) {
    // Apply immediate effects
    if (!choice.effects.is_null()) {
        rpg_utils::ApplyEffects(choice.effects, state_);
    }

    // Process checks if present
    if (!choice.check.is_null()) {
        const std::string stat = choice.check["type"].get<std::string>();
        const int player_stat = rpg_utils::CalculateStat(
            stat, state_.stats, data_);
        
        const int roll = rpg_utils::RollDice(3, 6); // 3d6 for skill checks
        const int result = player_stat - roll;

        // Determine outcome based on margin of success/failure
        std::string outcome_key = "fail";
        if (result >= 5) {
            outcome_key = "critical_success";
        } else if (result >= 0) {
            outcome_key = "success";
        } else if (result >= -5) {
            outcome_key = "fail";
        } else {
            outcome_key = "critical_fail";
        }

        // Get check outcome data
        const std::string& outcome_id = choice.check[outcome_key].get<std::string>();
        const auto& outcome_data = data_.Get("checks", outcome_id);
        
        // Process outcome
        Display(outcome_data);
        rpg_utils::ApplyEffects(outcome_data, state_);
        
        if (outcome_data.contains("next_scene")) {
            ProcessTransition(outcome_data["next_scene"].get<std::string>());
            return;
        }
        
        if (outcome_data.contains("choices")) {
            // If outcome has choices, process them recursively
            const auto& choices_json = outcome_data["choices"];
            std::vector<Choice> outcome_choices;
            
            for (const auto& c : choices_json) {
                Choice oc;
                oc.text = c["text"].get<std::string>();
                
                if (c.contains("check")) {
                    oc.check = c["check"];
                }
                
                if (c.contains("next_scene")) {
                    oc.next_target = c["next_scene"].get<std::string>();
                } else if (c.contains("success")) {
                    oc.next_target = c["success"].get<std::string>();
                } else if (c.contains("fail")) {
                    oc.next_target = c["fail"].get<std::string>();
                }
                
                outcome_choices.push_back(oc);
            }
            
            std::cout << "\n";
            for (size_t i = 0; i < outcome_choices.size(); ++i) {
                std::cout << i + 1 << ". " << outcome_choices[i].text << "\n";
            }
            
            int outcome_choice = rpg_utils::Input::GetInt(1, outcome_choices.size()) - 1;
            ExecuteChoice(outcome_choices[outcome_choice]);
            return;
        }
    }

    // Regular transition
    if (!choice.next_target.empty()) {
        ProcessTransition(choice.next_target);
    }
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