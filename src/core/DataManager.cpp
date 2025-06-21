#include "DataManager.h"
#include "Utils.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <cstdlib> // для getenv
#include <cctype> // для std::tolower
#include <algorithm> // для std::transform

// Для Windows API
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

void DataManager::LoadAll(const std::string& data_dir) {
    // Получаем абсолютный путь к директории данных
    const fs::path abs_path = fs::absolute(data_dir);

    // Проверяем существование директории
    if (!fs::is_directory(abs_path)) {
        throw std::runtime_error("Not a directory: " + abs_path.string());
    }

    data_path_ = abs_path.string();

    // Список обязательных файлов данных
    static constexpr std::array<const char*, 7> kDataFiles = {
        "character_base.json", "checks.json", "combat.json",
        "endings.json", "game_state.json", "items.json", "scenes.json"
    };

    for (const char* file_name : kDataFiles) {
        const fs::path file_path = abs_path / file_name;

        // Открываем файл
#ifdef _WIN32
        std::ifstream file(file_path.wstring());
#else
        std::ifstream file(file_path);
#endif

        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }

        // Загружаем JSON данные
        nlohmann::json data;
        try {
            file >> data;
        }
        catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON error in " + file_path.string() + ": " + e.what());
        }

        // Извлекаем имя файла без расширения для ключа
        const std::string key = std::string(file_name).substr(0, std::strlen(file_name) - 5);
        data_sets_[key] = std::move(data);
    }
}



const nlohmann::json& DataManager::Get(const std::string& type,
    const std::string& id) const {
    auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        throw std::out_of_range("Data type not found: " + type);
    }

    if (id.empty()) {
        return it->second;
    }

    if (!it->second.contains(id)) {
        throw std::out_of_range("ID not found in " + type + ": " + id);
    }

    return it->second[id];
}

nlohmann::json& DataManager::GetMutable(const std::string& type) {
    auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        throw std::out_of_range("Data type not found: " + type);
    }
    return it->second;
}

void DataManager::Save(const std::string& type) {
    auto it = data_sets_.find(type);
    if (it == data_sets_.end()) {
        throw std::out_of_range("Data type not found for save: " + type);
    }

    const fs::path file_path = fs::path(data_path_) / (type + ".json");
    std::ofstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + file_path.string());
    }

    try {
        file << it->second.dump(4);
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("JSON save error: " + std::string(e.what()));
    }
}