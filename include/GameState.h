// GameState.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "json.hpp"

struct GameState {
    std::string current_scene;
    bool quit_game = false;
    std::unordered_map<std::string, bool> flags;
    std::vector<std::string> inventory;
    std::set<std::string> unlocked_endings;

    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> derived_stats;
    int stat_points = 0;
    int current_health = 0;

    struct CombatState {
        std::string enemy_id;
        int enemy_health = 0;
        int current_phase = 0;
        bool player_turn = true;
    } combat;
};