#ifndef RPG_GAME_ENGINE_H_
#define RPG_GAME_ENGINE_H_

#include "DataManager.h"
#include "GameState.h"
#include "SceneProcessor.h"
#include "CombatProcessor.h"
#include "CharacterCreator.h"
#include "EndingProcessor.h"
#include <string>

// ��������������� ���������� ������ ������� ���������
class SceneProcessor;
class CombatProcessor;
class CharacterCreator;
class EndingProcessor;

class GameEngine {
public:
    explicit GameEngine(const std::string& data_path);
    ~GameEngine();
    void Run();

private:
    void InitializeGameState();
    void ProcessNextScene();
    void HandleCombatResult(const std::string& combat_id, bool player_won);
    void CleanupAfterScene();

    DataManager data_;
    GameState state_;
    SceneProcessor* scene_processor_;
    CombatProcessor* combat_processor_;
    CharacterCreator* character_creator_;
    EndingProcessor* ending_processor_;

    // ����������� ��� ������ ����� ����
    void ProcessNormalScene();
    void ProcessCombatScene();
    void ProcessCharacterCreation();
    void ProcessEndingScene();

    void ProcessMainMenu();
    void ProcessEndingsCollection();
};

#endif  // RPG_GAME_ENGINE_H_