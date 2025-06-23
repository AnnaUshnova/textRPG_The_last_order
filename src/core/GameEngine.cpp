#include "GameEngine.h"
#include "CharacterCreator.h"
#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>

GameEngine::GameEngine(const std::string& data_path)
    : data_(), state_() {
    data_.LoadAll(data_path);
    try {
        data_.ValidateData();
        std::cout << "All game data validated successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "DATA VALIDATION ERROR: " << e.what() << "\n";
    }

    // Создаем процессоры через new
    scene_processor_ = new SceneProcessor(data_, state_);
    combat_processor_ = new CombatProcessor(data_, state_);
    character_creator_ = new CharacterCreator(data_, state_);
    ending_processor_ = new EndingProcessor(data_, state_);
}


GameEngine::~GameEngine() {
    delete scene_processor_;
    delete combat_processor_;
    delete character_creator_;
    delete ending_processor_;
}

void GameEngine::InitializeGameState() {
    // Очистка предыдущего состояния
    state_.stats.clear();
    state_.derived_stats.clear();
    state_.current_health = 0;
    state_.stat_points = 0;

    try {
        const auto& char_data = data_.Get("character_base");

        // Инициализация базовых характеристик
        if (char_data.contains("base_stats")) {
            state_.stats = char_data["base_stats"].get<std::unordered_map<std::string, int>>();
        }

        // Инициализация очков распределения
        if (char_data.contains("points_to_distribute")) {
            state_.stat_points = char_data["points_to_distribute"].get<int>();
        }

        // Пересчет производных характеристик
        rpg_utils::CalculateDerivedStats(state_, char_data);

        // Отладочный вывод
        std::cout << "Характеристики персонажа инициализированы:\n";
        for (const auto& [stat, value] : state_.stats) {
            std::cout << "  " << stat << ": " << value << "\n";
        }
        std::cout << "Очков для распределения: " << state_.stat_points << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка инициализации персонажа: " << e.what() << "\n";

        // Значения по умолчанию
        state_.stats = {
            {"strength", 5},
            {"dexterity", 8},
            {"endurance", 8},
            {"intelligence", 8},
            {"melee", 8},
            {"ranged", 8}
        };
        state_.stat_points = 20;
    }

    state_.current_scene = "main_menu";
}


void GameEngine::Run() {
    InitializeGameState();

    // Проверяем, что начальная сцена установлена
    if (state_.current_scene.empty()) {
        state_.current_scene = "main_menu";
        std::cerr << "WARNING: Defaulting to main_menu scene\n";
    }

    while (!state_.quit_game) {
        CleanupAfterScene();

        std::cout << "Processing scene: " << state_.current_scene << "\n";

        if (state_.current_scene == "main_menu") {
            ProcessMainMenu();
        }
        else if (state_.current_scene == "endings_collection") {
            ProcessEndingsCollection();
        }
        else if (state_.current_scene == "character_creation") {
            ProcessCharacterCreation();
        }
        else {
            ProcessNextScene();
        }
    }
}

void GameEngine::ProcessNextScene() {
    const std::string& scene = state_.current_scene;

    if (scene.empty()) {
        std::cerr << "ERROR: Attempted to process empty scene ID\n";
        state_.current_scene = "main_menu";
        return;
    }

    std::cout << "Determining scene type for: " << scene << "\n";

    if (scene.find("combat_") == 0) {
        std::cout << "Identified as combat scene\n";
        ProcessCombatScene();
    }
    else if (scene.find("ending_") == 0) {
        std::cout << "Identified as ending scene\n";
        ProcessEndingScene();
    }
    else {
        std::cout << "Identified as normal scene\n";
        ProcessNormalScene();
    }
}

void GameEngine::ProcessMainMenu() {
    scene_processor_->Process("main_menu");
}

void GameEngine::ProcessEndingsCollection() {
    ending_processor_->ShowEndingCollection();
    state_.current_scene = "main_menu"; // Возврат в меню
}


void GameEngine::ProcessNormalScene() {
    scene_processor_->Process(state_.current_scene);

    // После обработки сцены обновляем текущую сцену
    // SceneProcessor устанавливает следующую сцену в GameState
}

void GameEngine::ProcessCombatScene() {
    const std::string combat_id = state_.current_scene;
    combat_processor_->Process(combat_id);

    // CombatProcessor должен установить результат боя в GameState
    // и следующую сцену в state_.current_scene
}

void GameEngine::ProcessCharacterCreation() {
    try {
        // Сброс предыдущего состояния персонажа
        state_.stats.clear();
        state_.derived_stats.clear();
        state_.current_health = 0;

        // Инициализация базовых характеристик
        const auto& char_data = data_.Get("character_base");
        if (char_data.contains("base_stats")) {
            state_.stats = char_data["base_stats"].get<std::unordered_map<std::string, int>>();
        }

        // Инициализация очков распределения
        if (char_data.contains("points_to_distribute")) {
            state_.stat_points = char_data["points_to_distribute"].get<int>();
        }

        // Пересчет производных характеристик
        rpg_utils::CalculateDerivedStats(state_, char_data);

        // Запуск процесса создания
        character_creator_->Process();

        // Переход к первой игровой сцене
        state_.current_scene = "scene1";
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка создания персонажа: " << e.what() << "\n";
        state_.current_scene = "main_menu";
    }
}

void GameEngine::ProcessEndingScene() {
    ending_processor_->Process(state_.current_scene);

    // После показа концовки завершаем игру
    state_.current_scene = "";
}

void GameEngine::HandleCombatResult(const std::string& combat_id, bool player_won) {
    const auto& combat_data = data_.Get("combat", combat_id);

    if (player_won) {
        state_.current_scene = combat_data["on_win"].get<std::string>();

        // Применяем эффекты победы
        if (combat_data.contains("on_win_effects")) {
            rpg_utils::ApplyGameEffects(combat_data["on_win_effects"], state_);
        }
    }
    else {
        state_.current_scene = combat_data["on_lose"].get<std::string>();

        // Разблокируем концовку
        if (combat_data.contains("on_lose_ending")) {
            state_.unlocked_endings.insert(
                combat_data["on_lose_ending"].get<std::string>()
            );
        }
    }
}

void GameEngine::CleanupAfterScene() {
    // Сбрасываем временные флаги
    state_.flags.erase("scene_transition");
    state_.flags.erase("choice_made");

}