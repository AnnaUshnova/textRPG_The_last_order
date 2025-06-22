#ifndef RPG_SCENE_PROCESSOR_H_
#define RPG_SCENE_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include "Utils.h"

class SceneProcessor {
public:
    SceneProcessor(DataManager& data, GameState& state);

    void Process(const std::string& scene_id);

private:
    struct Choice {
        std::string id;
        std::string text;
        nlohmann::json effects;
        nlohmann::json check;
        std::string next_target;
        std::string next_success;
        std::string next_fail;
        std::string next_scene;
        std::string flag_to_set; // Новое поле
    };


    void Display(const nlohmann::json& scene);
    std::vector<Choice> GetAvailableChoices(const nlohmann::json& scene);
    void ExecuteChoice(const Choice& choice);
    void ProcessTransition(const std::string& target);

    DataManager& data_;
    GameState& state_;
};

#endif  // RPG_SCENE_PROCESSOR_H_