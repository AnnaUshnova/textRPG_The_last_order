#ifndef RPG_GAME_STATE_H_
#define RPG_GAME_STATE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "json.hpp"

struct GameState {
    // ������� �������� �����
    std::string current_scene;

    // ���� ���������� ����
    bool quit_game = false;

    // ������������ ����� �����
    std::unordered_map<std::string, bool> flags;

    // ��������� ������
    std::vector<std::string> inventory;

    // ���������������� ��������
    std::set<std::string> unlocked_endings;

    // �������������� ��������� (����������� �� character_base.json)
    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> derived_stats;

    // ���� ��� ������������� (����������� �� character_base.json)
    int stat_points = 0;

    // ������� �������� ������ (�������������� �����������)
    int current_health = 0;

    // ��������� ������ ��� ������ ����
    struct CombatState {
        std::string enemy_id;           // ID �������� ����������
        int enemy_health = 0;           // ������� �������� ����������
        int current_phase = 0;          // ������� ���� ���
        bool player_turn = true;        // ��� ������ ���
        std::string last_action;        // ��������� ����������� ��������
    };

    CombatState combat;  // ��������� �������� ���
};

#endif  // RPG_GAME_STATE_H_