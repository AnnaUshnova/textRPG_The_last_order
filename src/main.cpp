#include "GameProcessor.h"
#include "DataManager.h"
#include "GameState.h"
#include <iostream>
#include <Windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        DataManager data;
        data.LoadAll("data/");

        GameState state;
        data.LoadGameState("save.json", state);

        // Упрощенная инициализация - только если сцена пустая
        if (state.current_scene.empty()) {
            state.current_scene = "main_menu";

            // Перенесено из InitializeCharacter
            const auto& char_base = data.Get("character_base");
            if (!char_base.is_null()) {
                state.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
                state.stat_points = char_base["points_to_distribute"].get<int>();

                // Используем единый метод расчета здоровья
                state.max_health = state.stats["endurance"] * 2;
                state.current_health = state.max_health;
            }
        }

        GameProcessor processor(data, state);

        while (!state.quit_game) {
            // Упрощенный обработчик сцен
            processor.ProcessScene(state.current_scene);
        }

        data.SaveGameState("save.json", state);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
