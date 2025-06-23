#include "DataManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void DataManager::LoadAll(const std::string& data_dir) {
    try {
        for (const auto& entry : fs::directory_iterator(data_dir)) {
            if (entry.path().extension() == ".json") {
                std::string type = entry.path().stem().string();
                LoadJsonFile(entry.path().string(), type);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading data: " << e.what() << std::endl;
    }
}

void DataManager::LoadJsonFile(const std::string& filename, const std::string& type) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        nlohmann::json data;
        file >> data;
        data_sets_[type] = data;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading " << type << " data: " << e.what() << std::endl;
    }
}

const nlohmann::json& DataManager::Get(const std::string& type) const {
    static const nlohmann::json empty = nlohmann::json::object();
    auto it = data_sets_.find(type);
    return (it != data_sets_.end()) ? it->second : empty;
}

nlohmann::json& DataManager::GetMutable(const std::string& type) {
    return data_sets_[type];
}