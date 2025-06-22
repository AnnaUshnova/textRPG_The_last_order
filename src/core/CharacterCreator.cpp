#include "CharacterCreator.h"
#include <iostream>
#include <vector>
#include <stdexcept>

CharacterCreator::CharacterCreator(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void CharacterCreator::Process() {
    // ������������� ������� �������������, ���� ��� ��� �� �����������
    if (state_.stats.empty()) {
        const auto& char_base = data_.Get("character_base");
        for (const auto& [stat, value] : char_base.items()) {
            if (stat != "points_to_distribute" && stat != "descriptions") {
                state_.stats[stat] = value.get<int>();
            }
        }
        state_.stat_points = char_base["points_to_distribute"].get<int>();
    }

    // �������� ���� ������������� �����
    while (state_.stat_points > 0) {
        DisplayStats();
        DistributePoints();
    }

    // ����� ������������� ��������� ���������
    state_.flags["character_created"] = true;
    std::cout << "\n�������� ��������� ���������!\n";
}

void CharacterCreator::DisplayStats() {
    const auto& char_base = data_.Get("character_base");
    const auto& descriptions = char_base["descriptions"];

    std::cout << "\n���� ��� �������������: " << state_.stat_points << "\n";
    std::cout << "��������������:\n";

    // ������� ������� ��������������
    for (const auto& stat : { "melee", "ranged", "strength", "endurance", "dexterity", "intelligence" }) {
        if (state_.stats.find(stat) != state_.stats.end()) {
            std::cout << "  " << stat << ": " << state_.stats.at(stat);
            if (descriptions.contains(stat)) {
                std::cout << " - " << descriptions[stat].get<std::string>();
            }
            std::cout << "\n";
        }
    }

    // ������� ����������� ��������������
    for (const auto& stat : { "health", "willpower" }) {
        if (state_.stats.find(stat) != state_.stats.end()) {
            std::cout << "  " << stat << ": " << state_.stats.at(stat);
            if (descriptions.contains(stat)) {
                std::cout << " - " << descriptions[stat].get<std::string>();
            }
            std::cout << "\n";
        }
    }
}

void CharacterCreator::DistributePoints() {
    // ������� ������ ��������� ��� ��������� �������������
    std::vector<std::string> upgradable_stats = {
        "melee", "ranged", "strength", "endurance", "dexterity", "intelligence"
    };

    // ������� ���� ������
    std::cout << "\n�������� �������������� ��� ���������:\n";
    for (size_t i = 0; i < upgradable_stats.size(); ++i) {
        std::cout << i + 1 << ". " << upgradable_stats[i] << "\n";
    }
    std::cout << upgradable_stats.size() + 1 << ". ��������� �������������\n";

    // �������� ����� ������
    int choice = rpg_utils::Input::GetInt(1, upgradable_stats.size() + 1);

    // ��������� ������
    if (choice <= static_cast<int>(upgradable_stats.size())) {
        const std::string& selected_stat = upgradable_stats[choice - 1];
        if (ApplyPoint(selected_stat)) {
            std::cout << selected_stat << " �������� �� 1 �����.\n";
        }
        else {
            std::cout << "������������ ����� ��� ���������.\n";
        }
    }
    else {
        state_.stat_points = 0; // ��������� �������������
    }
}

bool CharacterCreator::ApplyPoint(const std::string& stat) {
    if (state_.stat_points <= 0) return false;

    // ����������� ��������� ��������������
    state_.stats[stat]++;
    state_.stat_points--;

    // ������������� ����������� ��������������
    const auto& char_base = data_.Get("character_base");
    if (char_base.contains("derived_stats")) {
        const auto& derived_stats = char_base["derived_stats"];

        for (const auto& [derived_stat, formula] : derived_stats.items()) {
            try {
                std::string formula_str = formula.get<std::string>();
                auto tokens = rpg_utils::Split(rpg_utils::Trim(formula_str), ' ');

                if (tokens.size() == 3 && tokens[1] == "*") {
                    if (tokens[0] == stat) {
                        state_.stats[derived_stat] = state_.stats[stat] * std::stoi(tokens[2]);
                    }
                }
            }
            catch (...) {
                // ���������� ������ ������� ����������� �������������
            }
        }
    }

    return true;
}