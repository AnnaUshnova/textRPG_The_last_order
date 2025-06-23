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

    struct EnvironmentEffect {
        std::string name;
        std::string description;
        nlohmann::json effect;
    };

    // Основные этапы боя
    void InitializeCombat(const std::string& combat_id);
    void RunCombatLoop();
    void ProcessPlayerTurn();
    void ProcessEnemyTurn();
    void CleanupCombat(bool player_won);

    // Действия в бою
    void ExecutePlayerAction(const nlohmann::json& action);
    void ExecuteEnemyAttack(const nlohmann::json& attack);

    // Боевые расчеты

    int GetCurrentEnemyPhase() const;

    // Вспомогательные методы
    void DisplayCombatStatus() const;
    void DisplayPlayerOptions() const;
    void ApplyEnvironmentEffect(const EnvironmentEffect& effect);
    void ProcessDamage(int damage, bool target_is_player);

    DataManager& data_;
    GameState& state_;

    // Текущие боевые данные
    nlohmann::json combat_data_;
    std::vector<CombatPhase> enemy_phases_;
    std::vector<EnvironmentEffect> environment_effects_;
};

#endif // RPG_COMBAT_PROCESSOR_H_