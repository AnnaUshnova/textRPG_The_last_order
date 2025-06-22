#include "EndingProcessor.h"
#include "Utils.h"
#include <iostream>
#include <fstream>

EndingProcessor::EndingProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void EndingProcessor::Process(const std::string& ending_id) {
    // ��������� ������ ��������
    const auto& ending = data_.Get("endings", ending_id);

    // ���������� ��������
    ShowEnding(ending);

    // ��������� � ��������� �������� ��������
    state_.unlocked_endings.insert(ending_id);

    // ���������� ��������� ��������
    ShowEndingCollection();

    // ��������� ��������
    SaveProgress();

    // ��������� ��������� ����
    state_.active_type = "menu";
}

void EndingProcessor::ShowEnding(const nlohmann::json& ending) {
    std::cout << "\n\n=== " << ending["title"].get<std::string>() << " ===\n";
    std::cout << ending["text"].get<std::string>() << "\n";

    if (ending.contains("achievement")) {
        std::cout << "\n[����������: " << ending["achievement"].get<std::string>() << "]\n";
    }

    std::cout << "\n������� Enter, ����� ����������...";
    rpg_utils::Input::GetLine();
}

void EndingProcessor::ShowEndingCollection() {
    std::cout << "\n\n=== ��������� �������� ===\n";
    std::cout << "�������: " << state_.unlocked_endings.size() << " �� "
        << data_.Get("endings").size() << "\n\n";

    const auto& all_endings = data_.Get("endings");
    size_t counter = 1;

    for (const auto& [id, ending] : all_endings.items()) {
        std::cout << counter++ << ". ";

        if (state_.unlocked_endings.find(id) != state_.unlocked_endings.end()) {
            // �������� ��������
            std::cout << ending["title"].get<std::string>() << " - ";
            std::cout << ending["text"].get<std::string>() << "\n";
        }
        else {
            // ���������� ��������
            std::cout << "??? (����������� ��������)\n";
        }
    }

    std::cout << "\n������� Enter, ����� ��������� � ����...";
    rpg_utils::Input::GetLine();
}

void EndingProcessor::SaveProgress() {
    try {
        // ��������� ������ � ��������� � GameState
        nlohmann::json& game_state = data_.GetMutable("game_state");

        // ������� ������ �������� ��������
        nlohmann::json unlocked_array = nlohmann::json::array();
        for (const auto& ending_id : state_.unlocked_endings) {
            unlocked_array.push_back(ending_id);
        }

        // ��������� ������
        game_state["unlocked_endings"] = unlocked_array;

        // ��������� ������� � ����
        data_.Save("game_state");
    }
    catch (const std::exception& e) {
        std::cerr << "������ ���������� ���������: " << e.what() << std::endl;
    }
}