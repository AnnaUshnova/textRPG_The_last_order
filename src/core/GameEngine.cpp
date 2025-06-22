#include "GameEngine.h"
#include <stdexcept>

GameEngine::GameEngine(const std::string& data_path)
    : data_(),
    state_(),
    scene_processor_(data_, state_),
    combat_processor_(data_, state_),
    character_creator_(data_, state_),
    ending_processor_(data_, state_) {
    // Загрузка всех данных
    data_.LoadAll(data_path);

    // Инициализация состояния игры
    InitializeGameState();
}

void GameEngine::Run() {
    while (true) {
        if (state_.active_type == "scene") {
            scene_processor_.Process(state_.active_id);
        }
        else if (state_.active_type == "combat") {
            combat_processor_.Process(state_.combat_enemy);
        }
        else if (state_.active_type == "character_creation") {
            character_creator_.Process();
            // После создания персонажа переходим к следующей сцене
            state_.active_type = "scene";
            state_.active_id = "scene7";  // Первая игровая сцена
        }
        else if (state_.active_type == "ending") {
            ending_processor_.Process(state_.active_id);
            break;
        }
        else if (state_.active_type == "exit") {
            break;
        }
    }
}

void GameEngine::InitializeGameState() {
    const auto& game_state = data_.Get("game_state");
    state_.active_id = game_state["current_scene"].get<std::string>();


    // Определяем тип активной сцены по ID
    if (state_.active_id == "character_creation") {
        state_.active_type = "character_creation";
    }
    else if (state_.active_id.rfind("combat_", 0) == 0) {
        state_.active_type = "combat";
    }
    else if (state_.active_id.rfind("ending", 0) == 0) {
        state_.active_type = "ending";
    }
    else {
        state_.active_type = "scene";
    }

    // Загружаем характеристики
    if (game_state.contains("stats")) {
        for (const auto& [stat, value] : game_state["stats"].items()) {
            state_.stats[stat] = value.get<int>();
        }
    }

    // Загружаем только текущий инвентарь
    if (game_state.contains("inventory") && game_state["inventory"].is_array()) {
        for (const auto& item : game_state["inventory"]) {
            state_.inventory.push_back(item.get<std::string>());
        }
    }

    // Загружаем флаги
    if (game_state.contains("flags")) {
        for (const auto& [flag, value] : game_state["flags"].items()) {
            state_.flags[flag] = value.get<bool>();
        }
    }

    // Загружаем открытые концовки
    if (game_state.contains("unlocked_endings") && game_state["unlocked_endings"].is_array()) {
        for (const auto& ending : game_state["unlocked_endings"]) {
            state_.unlocked_endings.insert(ending.get<std::string>());
        }
    }
}

