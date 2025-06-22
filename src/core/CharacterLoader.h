#ifndef RPG_CHARACTER_LOADER_H_
#define RPG_CHARACTER_LOADER_H_

#include "GameState.h"
#include "json.hpp"

class CharacterLoader {
public:
    static void LoadFromJson(GameState& state, const nlohmann::json& data);
};

#endif  // RPG_CHARACTER_LOADER_H_