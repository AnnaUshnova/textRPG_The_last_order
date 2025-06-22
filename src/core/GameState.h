#ifndef RPG_GAME_STATE_H_
#define RPG_GAME_STATE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <set>


struct GameState {
    // Текущий активный элемент
    std::string active_id;
    std::string active_type;

    // Динамическое состояние
    std::unordered_map<std::string, bool> flags;
    std::vector<std::string> inventory;
    std::set<std::string> unlocked_endings;

    // Характеристики персонажа
    std::unordered_map<std::string, int> stats;
    std::unordered_map<std::string, int> resistances; // Добавлено сопротивление
    int stat_points = 0;
    int current_health = 0; // Единственное поле для здоровья игрока

    // Временные данные боя
    std::string combat_enemy;
    int combat_enemy_health = 0;
    bool player_turn = true;

    // Для отслеживания последней сцены
    std::string last_scene;
};

#endif  // RPG_GAME_STATE_H_