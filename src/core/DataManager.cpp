#include "DataManager.h"
#include "SceneProcessor.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

void DataManager::LoadAll(const std::string& data_dir) {
    data_sets_.clear();

    // Загрузка с явным указанием путей
    LoadJsonFile(data_dir + "/character_base.json", "character_base");
    LoadJsonFile(data_dir + "/scenes.json", "scenes");
    LoadJsonFile(data_dir + "/combats.json", "combats");
    LoadJsonFile(data_dir + "/endings.json", "endings");

    // Проверка загрузки
    for (const auto& type : { "character_base", "scenes", "combats", "endings" }) {
        if (data_sets_.find(type) == data_sets_.end()) {
            std::cerr << "WARNING: Failed to load " << type << " dataset\n";
        }
    }
}

void DataManager::LoadJsonFile(const std::string& filename, const std::string& type) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "ERROR: Can't open file: " << filename << "\n";
            return;
        }

        nlohmann::json data;
        file >> data;
        data_sets_[type] = data;
        std::cout << "Loaded: " << filename << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR loading " << filename << ": " << e.what() << "\n";
    }
}



const nlohmann::json& DataManager::Get(const std::string& type) const {
    auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        std::cerr << "ERROR: Requested data type not found: " << type << "\n";
        static nlohmann::json empty = nlohmann::json::object();
        return empty;
    }
    return it->second;
}

const nlohmann::json& DataManager::Get(const std::string& type, const std::string& id) const {
    const auto& dataset = Get(type);

    if (!dataset.contains(id)) {
        std::cerr << "ERROR: ID '" << id << "' not found in dataset '" << type << "'\n";
        std::cerr << "Available IDs: ";
        for (const auto& [key, value] : dataset.items()) {
            std::cerr << key << " ";
        }
        std::cerr << "\n";

        static nlohmann::json empty = nlohmann::json::object();
        return empty;
    }

    return dataset[id];
}




void DataManager::ValidateData() const {
    const std::vector<std::string> required_datasets = {
        "character_base", "scenes", "combats", "endings"
    };

    for (const auto& type : required_datasets) {
        if (data_sets_.find(type) == data_sets_.end()) {
            throw std::runtime_error("Missing required dataset: " + type);
        }
    }

    // Проверка базовых характеристик персонажа
    const auto& char_data = Get("character_base");
    if (!char_data.contains("base_stats") || char_data["base_stats"].empty()) {
        throw std::runtime_error("character_base.json has invalid 'base_stats'");
    }
}

