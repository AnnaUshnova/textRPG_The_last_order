#ifndef RPG_CHARACTER_CREATOR_H_
#define RPG_CHARACTER_CREATOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"
#include <vector>
#include <string>
#include <unordered_map>

class CharacterCreator {
public:
    CharacterCreator(DataManager& data, GameState& state);
    void Process();

private:
    struct CharacterConfig {
        std::vector<std::string> core_stats;
        std::unordered_map<std::string, std::string> display_names;
        std::unordered_map<std::string, std::string> descriptions;
    };

    void ShowWelcomeScreen();
    void DistributePoints();
    void FinalizeCharacter();
    void ShowSummary();
    void DisplayStats(bool show_descriptions = true);
    CharacterConfig LoadConfig() const;
    void ApplyPointToStat(const std::string& stat);
    void CalculateDerivedStats();
    int GetStatIndexInput() const;

    DataManager& data_;
    GameState& state_;
};

#endif  // RPG_CHARACTER_CREATOR_H_