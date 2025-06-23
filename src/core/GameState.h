#ifndef RPG_GAME_STATE_H_
#define RPG_GAME_STATE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "json.hpp"

struct GameState {
    // Текущая активная сцена
    std::string current_scene;

    // Флаг завершения игры
    bool quit_game = false;

    // Динамические флаги сцены
    std::unordered_map<std::string, bool> flags;

    // Инвентарь игрока
    std::vector<std::string> inventory;

    // Разблокированные концовки
    std::set<std::string> unlocked_endings;

    // Характеристики персонажа (загружаются из character_base.json)
    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> derived_stats;

    // Очки для распределения (загружаются из character_base.json)
    int stat_points = 0;

    // Текущее здоровье игрока (рассчитывается динамически)
    int current_health = 0;

    // Временные данные для боевых сцен
    struct CombatState {
        std::string enemy_id;           // ID текущего противника
        int enemy_health = 0;           // Текущее здоровье противника
        int current_phase = 0;          // Текущая фаза боя
        bool player_turn = true;        // Чей сейчас ход
        std::string last_action;        // Последнее выполненное действие
    };

    CombatState combat;  // Состояние текущего боя
};

#endif  // RPG_GAME_STATE_H_