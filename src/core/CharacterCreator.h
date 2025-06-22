#ifndef RPG_CHARACTER_CREATOR_H_
#define RPG_CHARACTER_CREATOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"

class CharacterCreator {
public:
	CharacterCreator(DataManager& data, GameState& state);

	void Process();

private:
	void DisplayStats();
	void DistributePoints();
	bool ApplyPoint(const std::string& stat);

	DataManager& data_;
	GameState& state_;
};

#endif  // RPG_CHARACTER_CREATOR_H_