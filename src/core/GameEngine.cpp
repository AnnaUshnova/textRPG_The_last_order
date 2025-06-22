#include "GameEngine.h"
#include <stdexcept>
#include <iostream>


GameEngine::GameEngine(const std::string& data_path)
    : data_(),
    state_(),
    scene_processor_(data_, state_),
    combat_processor_(data_, state_),
    character_creator_(data_, state_),
    ending_processor_(data_, state_) {
    // �������� ���� ������
    data_.LoadAll(data_path);

    // ������������� ��������� ����
    InitializeGameState();
}

void GameEngine::Run() {                                                   
    std::cout << "=== ������ ���� ===\n";

    // �������� ������� ����
    while (true) {
        if (state_.active_type == "scene") {
            scene_processor_.Process(state_.active_id);
        }
        else if (state_.active_type == "combat") {
            combat_processor_.Process(state_.combat_enemy);
        }
        else if (state_.active_type == "character_creation") {
            character_creator_.Process();
            // ����� �������� ��������� ��������� � ��������� �����
            state_.active_type = "scene";
            state_.active_id = "scene7";  // ������ ������� �����
        }
        else if (state_.active_type == "ending") {
            ending_processor_.Process(state_.active_id);
            // ����� �������� ��������� ����
            std::cout << "\n���� ���������. ������� �� �����������!\n";
            break;
        }
        else if (state_.active_type == "exit") {
            std::cout << "\n����� �� ����...\n";
            break;
        }
        else {
            throw std::runtime_error("Unknown game state: " + state_.active_type);
        }
    }
}

void GameEngine::InitializeGameState() {
    // ��������� ��������� ��������� �� JSON
    const auto& game_state = data_.Get("game_state");

    // ������������� ������� �����
    state_.active_type = "scene";
    state_.active_id = game_state["current_scene"].get<std::string>();

    // ��������� ��������������
    if (game_state.contains("stats")) {
        for (const auto& [stat, value] : game_state["stats"].items()) {
            state_.stats[stat] = value.get<int>();
        }
    }

    // �������������� �������� �� �������������
    if (state_.stats.find("health") != state_.stats.end()) {
        state_.current_health = state_.stats["health"];
    }
    else {
        // �������� �� ���������, ���� �������������� �� �������
        state_.current_health = 100;
    }

    // ��������� ���������
    if (game_state.contains("inventory") && game_state["inventory"].is_array()) {
        for (const auto& item : game_state["inventory"]) {
            state_.inventory.push_back(item.get<std::string>());
        }
    }

    // ��������� �����
    if (game_state.contains("flags")) {
        for (const auto& [flag, value] : game_state["flags"].items()) {
            state_.flags[flag] = value.get<bool>();
        }
    }

    // ��������� �������� ��������
    if (game_state.contains("unlocked_endings") && game_state["unlocked_endings"].is_array()) {
        for (const auto& ending : game_state["unlocked_endings"]) {
            state_.unlocked_endings.insert(ending.get<std::string>());
        }
    }

    // ���������, ����� �� ��������� ���������
    if (state_.active_id == "character_creation") {
        state_.active_type = "character_creation";
    }
}