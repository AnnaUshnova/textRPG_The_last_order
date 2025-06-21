#include "core/GameEngine.h"
#include <Windows.h>
#include <iostream>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        // Вывод текущей директории
        std::cout << "Current working directory: "
            << std::filesystem::current_path().string() << "\n";

        GameEngine game("src/data/");
        game.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}