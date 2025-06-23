#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct CombatState {
    std::string enemy_id;
    int enemy_health = 0;
    int current_phase = 0;
    bool player_turn = true;
};

struct GameState {
    std::string current_scene = "main_menu";
    int current_health = 0;
    int stat_points = 0;
    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> derived_stats;
    std::vector<std::string> inventory;
    std::unordered_map<std::string, bool> flags;
    std::unordered_set<std::string> unlocked_endings;
    bool quit_game = false;

    CombatState combat;
};