#include "GameProcessor.h"
#include "DataManager.h"
#include "GameState.h"
#include <iostream>
#include <Windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        // Инициализация менеджера данных
        DataManager data;
        data.LoadAll("data/");

        // Инициализация состояния игры
        GameState state;

        // Загрузка сохраненной игры, если есть
        data.LoadGameState("save.json", state);
        std::cout << "Текущая сцена после загрузки: " << state.current_scene << "\n";
        std::cout << "Здоровье после загрузки: " << state.current_health << "\n";

        GameProcessor processor(data, state);

        while (!state.quit_game) {
            const std::string& current = state.current_scene;

            // Обработка специальных сцен
            if (current == "quit_game") {
                state.quit_game = true;
            }
            else if (current.find("combat_") == 0) {
                processor.ProcessCombat(current);
            }
            else if (current == "character_creation") {
                processor.InitializeCharacter();
                state.current_scene = "scene7";
            }
            else if (current == "endings_collection") {
                processor.ShowEndingCollection();
                state.current_scene = "main_menu";
            }
            else if (current.find("ending") == 0) {
                processor.ShowEnding(current);
                state.current_scene = "main_menu"; // Возврат в главное меню после концовки
            }
            else {
                processor.ProcessScene(current);
            }
        }

        // Сохранение игры перед выходом
        state.quit_game = false; // Сбрасываем флаг выхода для следующего запуска
        data.SaveGameState("save.json", state);
        std::cout << "Прогресс сохранен. До свидания!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}