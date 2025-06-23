// DataManager.h
#pragma once
#include <string>
#include <unordered_map>
#include "json.hpp"

class DataManager {
public:
    void LoadAll(const std::string& data_dir);
    const nlohmann::json& Get(const std::string& type) const;
    nlohmann::json& GetMutable(const std::string& type);

private:
    std::unordered_map<std::string, nlohmann::json> data_sets_;
    void LoadJsonFile(const std::string& filename, const std::string& type);
};