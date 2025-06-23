#pragma once
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

    // Новые функции для работы с сохранениями
    void SaveGameState(const std::string& filename, const GameState& state);
    void LoadGameState(const std::string& filename, GameState& state);

    // Функция для сброса состояния к начальному
    void ResetGameState(GameState& state, const nlohmann::json& char_base);

private:
    std::unordered_map<std::string, nlohmann::json> data_sets_;
};