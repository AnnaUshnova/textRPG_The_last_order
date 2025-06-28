#include "GameProcessor.h"
#include "DataManager.h"
#include "GameState.h"

#include <iostream>

#include <Windows.h>

int main() {
    // Настройка кодировки консоли для поддержки UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Включение поддержки виртуальных терминалов для цветного вывода
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode;
    if (GetConsoleMode(h_console, &console_mode)) {
        console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(h_console, console_mode);
    }

    try {
        DataManager data;
        data.LoadAll("data/");

        GameState state;
        data.LoadGameState("save.json", state);

        GameProcessor processor(data, state);

        // Основной игровой цикл
        while (!state.quit_game) {
            processor.ProcessScene(state.current_scene);
        }

        // Сохранение состояния при выходе
        data.SaveGameState("save.json", state);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}