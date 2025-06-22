#ifndef RPG_COMBAT_PROCESSOR_H_
#define RPG_COMBAT_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"

class CombatProcessor {
public:
    enum class RollResult {
        kCriticalSuccess,
        kSuccess,
        kFail,
        kCriticalFail
    };

    CombatProcessor(DataManager& data, GameState& state);

    void Process(const std::string& combat_id);

private:
    void InitializeCombat(const nlohmann::json& combat);
    void PlayerTurn(const nlohmann::json& combat);
    void EnemyTurn(const nlohmann::json& combat);
    void ResolveAction(const nlohmann::json& action);
    RollResult RollCheck(const std::string& stat_name) const;
    void ApplyCombatEffects(const nlohmann::json& effects);

    DataManager& data_;
    GameState& state_;
};

#endif  // RPG_COMBAT_PROCESSOR_H_