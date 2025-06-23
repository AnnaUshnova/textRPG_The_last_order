// GameProcessor.h
#pragma once
#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"
#include <string>
#include <vector>

class GameProcessor {
public:
    GameProcessor(DataManager& data, GameState& state);

    // Основные игровые процессы
    void ProcessScene(const std::string& scene_id);
    void ProcessCombat(const std::string& combat_id);
    void ShowEnding(const std::string& ending_id);
    void ShowEndingCollection();
    void InitializeCharacter();

private:
    struct SceneChoice {
        std::string id;
        std::string text;
        nlohmann::json effects;
        nlohmann::json check;
        std::string next_target;

    };

    // Вспомогательные методы для сцен
    void DisplayScene(const nlohmann::json& scene);
    void HandleAutoAction(const std::string& action);
    void ProcessPlayerChoice(const std::vector<SceneChoice>& choices);
    void ProcessCheck(const nlohmann::json& check_data);
    void ProcessCheckResult(const nlohmann::json& result);
    std::vector<SceneChoice> GetAvailableChoices(const nlohmann::json& choices);
    bool EvaluateCondition(const std::string& condition);
    bool EvaluateSingleCondition(const std::string& condition);
    void ResetVisitedScenes();

    // Методы для боевой системы
    void InitializeCombat(const nlohmann::json& combat_data, const std::string& combat_id);
    void DisplayCombatStatus(const nlohmann::json& combat_data);
    void ProcessPlayerTurn(const nlohmann::json& combat_data);
    void ProcessEnemyTurn(const nlohmann::json& combat_data);
    void CleanupCombat(const nlohmann::json& combat_data);
    std::string ResultTypeToString(rpg_utils::RollResultType type);
    std::unordered_map<std::string, bool> visited_scenes_;

    // Общие утилиты
    void ApplyGameEffects(const nlohmann::json& effects);
    void CalculateDerivedStats();
    void DisplayEnding(const nlohmann::json& ending);

    DataManager& data_;
    GameState& state_;
};