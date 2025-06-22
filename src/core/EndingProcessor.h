#ifndef RPG_ENDING_PROCESSOR_H_
#define RPG_ENDING_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"

class EndingProcessor {
public:
	EndingProcessor(DataManager& data, GameState& state);

	void Process(const std::string& ending_id);

private:
	void ShowEnding(const nlohmann::json& ending);
	void ShowEndingCollection();
	void SaveProgress();

	DataManager& data_;
	GameState& state_;
};

#endif  // RPG_ENDING_PROCESSOR_H_