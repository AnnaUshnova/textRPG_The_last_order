#ifndef RPG_SCENE_PROCESSOR_H_
#define RPG_SCENE_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"
#include "EndingProcessor.h"
#include <vector>
#include <string>
#include <unordered_map>

class SceneProcessor {
public:
    SceneProcessor(DataManager& data, GameState& state);
    void Process(const std::string& scene_id);

    struct SceneChoice {
        std::string id;
        std::string text;
        nlohmann::json effects;
        nlohmann::json check;
        std::string next_target;
    };

private:
    void DisplayScene(const nlohmann::json& scene);
    std::vector<SceneChoice> GetAvailableChoices(const nlohmann::json& choices);
    void ProcessAutoCheck(const nlohmann::json& check_data);
    void ProcessPlayerChoice(const SceneChoice& choice);
    std::string DetermineNextScene(const nlohmann::json& check_data, int roll_result);

    DataManager& data_;
    GameState& state_;
    EndingProcessor ending_processor_;
};

#endif  // RPG_SCENE_PROCESSOR_H_