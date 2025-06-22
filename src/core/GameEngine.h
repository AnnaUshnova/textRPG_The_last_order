#ifndef RPG_GAME_ENGINE_H_
#define RPG_GAME_ENGINE_H_

#include "DataManager.h"
#include "GameState.h"
#include "SceneProcessor.h"
#include "CombatProcessor.h"
#include "CharacterCreator.h"
#include "EndingProcessor.h"
#include <string>

class GameEngine {
public:
	explicit GameEngine(const std::string& data_path);
	void Run();

private:
	void InitializeGameState();

	DataManager data_;
	GameState state_;
	SceneProcessor scene_processor_;
	CombatProcessor combat_processor_;
	CharacterCreator character_creator_;
	EndingProcessor ending_processor_;
};

#endif  // RPG_GAME_ENGINE_H_