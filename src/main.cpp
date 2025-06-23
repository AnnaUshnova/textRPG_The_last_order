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
        state.current_scene = "main_menu";

        GameProcessor processor(data, state);

        while (!state.quit_game) {
            const std::string& current = state.current_scene;

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
            }
            else {
                processor.ProcessScene(current);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}