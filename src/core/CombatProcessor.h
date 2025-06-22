#ifndef RPG_COMBAT_PROCESSOR_H_
#define RPG_COMBAT_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"

class CombatProcessor {
public:
    CombatProcessor(DataManager& data, GameState& state);
    void Process(const std::string& combat_id);

private:
    struct CombatPhase {
        int health_threshold;
        std::vector<nlohmann::json> attacks;
    };

    void InitializeCombat(const nlohmann::json& combat_data);
    void PlayerTurn();
    void EnemyTurn();
    void ExecutePlayerAction(const nlohmann::json& action);
    void ExecuteEnemyAttack(const nlohmann::json& attack);
    void ResolveAttack(const nlohmann::json& attack, bool is_player_attacking);
    void ProcessCombatResult(bool player_won);
    int CalculateDamage(const std::string& dice_formula);
    int GetCurrentEnemyPhase();

    DataManager& data_manager_;
    GameState& game_state_;
    nlohmann::json current_combat_;
    std::vector<CombatPhase> enemy_phases_;
    std::string next_scene_;  // Локальная переменная для хранения следующей сцены
};

#endif // RPG_COMBAT_PROCESSOR_H_