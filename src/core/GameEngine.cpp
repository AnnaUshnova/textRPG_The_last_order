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
    // Загрузка всех данных
    data_.LoadAll(data_path);

    // Инициализация состояния игры
    InitializeGameState();
}

void GameEngine::Run() {                                                   
    std::cout << "=== ЗАПУСК ИГРЫ ===\n";

    // Основной игровой цикл
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
            // После концовки завершаем игру
            std::cout << "\nИгра завершена. Спасибо за прохождение!\n";
            break;
        }
        else if (state_.active_type == "exit") {
            std::cout << "\nВыход из игры...\n";
            break;
        }
        else {
            throw std::runtime_error("Unknown game state: " + state_.active_type);
        }
    }
}

void GameEngine::InitializeGameState() {
    // Загружаем начальное состояние из JSON
    const auto& game_state = data_.Get("game_state");

    // Устанавливаем текущую сцену
    state_.active_type = "scene";
    state_.active_id = game_state["current_scene"].get<std::string>();

    // Загружаем характеристики
    if (game_state.contains("stats")) {
        for (const auto& [stat, value] : game_state["stats"].items()) {
            state_.stats[stat] = value.get<int>();
        }
    }

    // Инициализируем здоровье из характеристик
    if (state_.stats.find("health") != state_.stats.end()) {
        state_.current_health = state_.stats["health"];
    }
    else {
        // Значение по умолчанию, если характеристика не найдена
        state_.current_health = 100;
    }

    // Загружаем инвентарь
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

    // Проверяем, нужно ли создавать персонажа
    if (state_.active_id == "character_creation") {
        state_.active_type = "character_creation";
    }
}