#include "GameProcessor.h"
#include "DataManager.h"
#include "GameState.h"
#include <iostream>
#include <Windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        // ������������� ��������� ������
        DataManager data;
        data.LoadAll("data/");

        // ������������� ��������� ����
        GameState state;

        // �������� ����������� ����, ���� ����
        data.LoadGameState("save.json", state);
        std::cout << "������� ����� ����� ��������: " << state.current_scene << "\n";
        std::cout << "�������� ����� ��������: " << state.current_health << "\n";

        GameProcessor processor(data, state);

        while (!state.quit_game) {
            const std::string& current = state.current_scene;

            // ��������� ����������� ����
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
                state.current_scene = "main_menu"; // ������� � ������� ���� ����� ��������
            }
            else {
                processor.ProcessScene(current);
            }
        }

        // ���������� ���� ����� �������
        state.quit_game = false; // ���������� ���� ������ ��� ���������� �������
        data.SaveGameState("save.json", state);
        std::cout << "�������� ��������. �� ��������!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}