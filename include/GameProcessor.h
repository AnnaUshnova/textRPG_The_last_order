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

    // �������� ������� ��������
    void ProcessScene(const std::string& scene_id);
    void ProcessCombat(const std::string& combat_id);
    void ShowEnding(const std::string& ending_id);
    void ShowEndingCollection();
    void InitializeCharacter();

private:
    struct SceneChoice {
        int number;  // ������������ ��� ������ ������
        std::string text;
        nlohmann::json data;  // �������� ��� ������ ������
    };
    struct CombatOption {
        int number;
        std::string text;
        nlohmann::json data;
        std::string type;
    };


    // ��������������� ������
    void ProcessSceneChoices(const nlohmann::json& choices);
    void ProcessCheck(const nlohmann::json& check_data);
    void ApplyGameEffects(const nlohmann::json& effects);
    bool EvaluateCondition(const std::string& condition);
    void HandleAutoAction(const std::string& action);
    void CalculateDerivedStats();
    void DisplayEnding(const nlohmann::json& ending);
    void ProcessPlayerCombatTurn(const nlohmann::json& combat);
    void ApplyCombatResults(const nlohmann::json& results);
    void InitializeCombat(const nlohmann::json& combat_data, const std::string& combat_id);
    void ProcessEnemyCombatTurn(const nlohmann::json& combat);
    void CleanupCombat(const nlohmann::json& combat_data);
    void DisplayCombatStatus(const nlohmann::json& combat_data);
    void InitializeNewGame();
    void StartNewGame();
    void FullReset();
    void UseInventoryItemInCombat();
    void ShowInventory();
    void AddItemToInventory(const std::string& item_id);
    void RemoveItemFromInventory(const std::string& item_id);
    bool HasItem(const std::string& item_id) const;
    void UseItemOutsideCombat();
    void UseCombatInventory();
    void ApplyInventoryItemEffects(const std::string& item_id, const nlohmann::json& item_data);




    DataManager& data_;
    GameState& state_;
};