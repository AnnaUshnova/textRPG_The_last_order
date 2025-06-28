#pragma once
#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"
#include <json.hpp>
#include <vector>
#include <string>

class GameProcessor {
public:
    GameProcessor(DataManager& data, GameState& state);

    // Основные игровые функции
    void ProcessScene(const std::string& scene_id);
    void ShowEnding(const std::string& ending_id);
    void ShowEndingCollection();
    void ProcessCombat(const std::string& combat_id);
    void StartNewGame();
    void FullReset();
    void InitializeNewGame();

    // Функции управления состоянием
    void CalculateDerivedStats();
    void ApplyGameEffects(const nlohmann::json& effects);
    void HandleAutoAction(const std::string& action);
    void InitializeCharacter();

    // Функции инвентаря
    void AddItemToInventory(const std::string& item_id, int count = 1);
    void RemoveItemFromInventory(const std::string& item_id, int count = 1);
    bool HasItem(const std::string& item_id, int count = 1) const;
    void ShowInventory();
    void UseItemOutsideCombat();
    void UseCombatInventory();

private:
    // Структуры для внутреннего использования
    struct SceneChoice {
        int number;
        std::string text;
        nlohmann::json data;
    };

    struct CombatOption {
        int number;
        std::string text;
        nlohmann::json data;
        std::string type;
    };

    // Основные обработчики
    void ProcessSceneChoices(const nlohmann::json& choices);
    void ProcessCheck(const nlohmann::json& check_data);
    void ProcessPlayerCombatTurn(const nlohmann::json& combat);
    void ProcessEnemyCombatTurn(const nlohmann::json& combat);

    // Боевая система
    void InitializeCombat(const nlohmann::json& combat_data, const std::string& combat_id);
    void DisplayCombatStatus(const nlohmann::json& combat_data);
    void ApplyInventoryItemEffects(const std::string& item_id, const nlohmann::json& item_data);
    void CleanupCombat(const nlohmann::json& combat_data);
    void HandleCombatVictory(const nlohmann::json& combat_data);
    void HandleCombatDefeat(const nlohmann::json& combat_data);

    // Вспомогательные функции
    bool EvaluateCondition(const std::string& condition);
    void ApplyCombatResults(const nlohmann::json& results);  // Удалить если не используется

    DataManager& data_;
    GameState& state_;
};