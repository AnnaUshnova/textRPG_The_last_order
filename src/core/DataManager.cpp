#include "DataManager.h"
#include "Utils.h"
#include <fstream>
#include <stdexcept>

namespace {
    const std::unordered_map<std::string, std::string> kDataFiles = {
        {"character_base", "character_base.json"},
        {"checks", "checks.json"},
        {"combat", "combat.json"},
        {"endings", "endings.json"},
        {"game_state", "game_state.json"},
        {"items", "items.json"},
        {"scenes", "scenes.json"}
    };
}  // namespace

void DataManager::LoadAll(const std::string& data_dir) {
    data_path_ = data_dir;
    if (data_path_.back() != '/') {
        data_path_ += '/';
    }

    for (const auto& [data_type, filename] : kDataFiles) {
        const std::string full_path = data_path_ + filename;
        std::ifstream file(full_path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open data file: " + full_path);
        }

        try {
            data_sets_[data_type] = nlohmann::json::parse(file);
        }
        catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error in " + filename +
                ": " + e.what());
        }
    }
}

const nlohmann::json& DataManager::Get(const std::string& type,
    const std::string& id) const {
    const auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        throw std::out_of_range("Unknown data type: " + type);
    }

    if (id.empty()) {
        return it->second;
    }

    if (!it->second.contains(id)) {
        throw std::out_of_range("ID not found: " + id + " in type: " + type);
    }

    return it->second.at(id);
}

nlohmann::json& DataManager::GetMutable(const std::string& type) {
    auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        throw std::out_of_range("Unknown data type: " + type);
    }
    return it->second;
}

void DataManager::Save(const std::string& type) {
    const auto file_it = kDataFiles.find(type);
    if (file_it == kDataFiles.end()) {
        throw std::out_of_range("Unknown data type for save: " + type);
    }

    const std::string full_path = data_path_ + file_it->second;
    std::ofstream file(full_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + full_path);
    }

    file << data_sets_.at(type).dump(2);  // Pretty print with 2-space indent
}