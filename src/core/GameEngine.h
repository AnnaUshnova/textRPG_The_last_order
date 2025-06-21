#ifndef RPG_GAME_ENGINE_H_
#define RPG_GAME_ENGINE_H_

#include "DataManager.h"
#include "GameState.h"
#include <string>

class GameEngine {
public:
    explicit GameEngine(const std::string& data_path);
    void Run();

private:
    enum class RollResult {
        kCriticalSuccess,
        kSuccess,
        kFail,
        kCriticalFail
    };

    void ProcessScene(const std::string& scene_id);
    void ProcessCombat(const std::string& combat_id);
    void ProcessEnding(const std::string& ending_id);
    void ProcessCharacterCreation();

    void ApplyEffects(const nlohmann::json& effects);
    bool EvaluateCondition(const std::string& condition) const;
    int CalculateStat(const std::string& stat_name) const;

    RollResult RollCheck(const std::string& stat_name) const;
    void ResolveCombatAction(const nlohmann::json& action);

    DataManager data_;
    GameState state_;
};

#endif  // RPG_GAME_ENGINE_H_