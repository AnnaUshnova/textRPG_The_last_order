#ifndef RPG_CHARACTER_CREATOR_H_
#define RPG_CHARACTER_CREATOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"
#include <string>

class CharacterCreator {
public:
    CharacterCreator(DataManager& data, GameState& state);
    void Process();

private:
    void InitializeStats();
    void DisplayStats(bool full_descriptions);
    void DistributePoints();
    void FinalizeCreation();

    int CalculateMaxNameLength() const;
    bool ShouldDisplayStat(const std::string& stat) const;
    void DisplaySingleStat(
        const std::string& stat,
        const nlohmann::json& display_names,
        const nlohmann::json& name_lengths,
        const nlohmann::json& descriptions,
        int max_name_length,
        bool show_description
    );

    std::string GetStatByIndex(int index);
    bool ApplyPoints(const std::string& stat, int points);
    bool ApplySinglePoint(const std::string& stat);

    DataManager& data_;
    GameState& state_;
    bool first_display_;  // Флаг для отслеживания первого отображения
};

#endif  // RPG_CHARACTER_CREATOR_H_