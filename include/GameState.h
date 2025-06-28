#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct CombatState {
    std::string enemy_id;
    int enemy_health = 0;
    int max_enemy_health = 0;
    int current_phase = 0;
    bool player_turn = true;
    std::string last_enemy_action;
};

struct GameState {
    std::string current_scene = "main_menu";
    int current_health = 0;
    int max_health = 0;
    int stat_points = 0;
    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> derived_stats;
    std::unordered_map<std::string, int> inventory;
    std::unordered_map<std::string, bool> flags;
    std::unordered_set<std::string> unlocked_endings;
    bool quit_game = false;
    std::unordered_map<std::string, std::string> string_vars;
    std::unordered_map<std::string, bool> visited_scenes;

    CombatState combat;
};