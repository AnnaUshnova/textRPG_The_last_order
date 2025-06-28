#include "GameProcessor.h"
#include "DataManager.h"
#include "GameState.h"
#include <iostream>
#include <Windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Включаем поддержку виртуальных терминалов для цветов
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, consoleMode);

    try {
        DataManager data;
        data.LoadAll("data/");

        GameState state;
        data.LoadGameState("save.json", state);

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