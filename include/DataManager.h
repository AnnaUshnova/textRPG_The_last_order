// DataManager.h
#ifndef DATAMANAGER_H_
#define DATAMANAGER_H_

#include <json.hpp>
#include <string>
#include <unordered_map>

#include "GameState.h"

class DataManager {
public:
	void LoadAll(const std::string& data_dir);
	void LoadJsonFile(const std::string& filename, const std::string& type);
	const nlohmann::json& Get(const std::string& type) const;
	nlohmann::json& GetMutable(const std::string& type);

	// Save management
	void SaveGameState(const std::string& filename, const GameState& state);
	void LoadGameState(const std::string& filename, GameState& state);
	void ResetGameState(GameState& state);

	nlohmann::json GetItem(const std::string& item_id) const;

private:
	std::unordered_map<std::string, nlohmann::json> data_sets_;
};

#endif  // DATAMANAGER_H_