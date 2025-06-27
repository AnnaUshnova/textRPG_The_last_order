#include "DataManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void DataManager::LoadAll(const std::string& data_dir) {
    std::cout << "Загрузка данных из: " << data_dir << "\n";

    try {
        for (const auto& entry : fs::directory_iterator(data_dir)) {
            std::cout << "Найден файл: " << entry.path() << "\n";
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

        // Если есть раздел с именем типа, используем его
        if (data.contains(type)) {
            data_sets_[type] = data[type];
        }
        // Иначе используем весь файл
        else {
            data_sets_[type] = data;
        }

        // Отладочный вывод
        std::cout << "Загружены данные типа: " << type << "\n";
        std::cout << "Количество элементов: " << data_sets_[type].size() << "\n";
        std::cout << "Ключи: ";
        for (auto& [key, value] : data_sets_[type].items()) {
            std::cout << key << ", ";
        }
        std::cout << "\n";
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

void DataManager::SaveGameState(const std::string& filename, const GameState& state) {
    nlohmann::json j;

    // Сохраняем только концовки
    j["unlocked_endings"] = nlohmann::json::array();
    for (const auto& ending : state.unlocked_endings) {
        j["unlocked_endings"].push_back(ending);
    }

    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void DataManager::LoadGameState(const std::string& filename, GameState& state) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    try {
        nlohmann::json j;
        file >> j;

        // Загружаем только концовки
        if (j.contains("unlocked_endings")) {
            state.unlocked_endings.clear();
            for (const auto& item : j["unlocked_endings"]) {
                state.unlocked_endings.insert(item.get<std::string>());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки сохранений: " << e.what() << "\n";
    }
}

void DataManager::ResetGameState(GameState& state) {
    // Сохраняем только концовки
    std::unordered_set<std::string> saved_endings = state.unlocked_endings;

    // Полностью сбрасываем состояние
    state = GameState();

    // Восстанавливаем концовки
    state.unlocked_endings = saved_endings;

    // Загружаем базовые характеристики
    const auto& char_base = Get("character_base");
    state.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
    state.stat_points = char_base["points_to_distribute"].get<int>();

    // Рассчитываем здоровье
    if (char_base.contains("derived_stats_formulas")) {
        const auto& formulas = char_base["derived_stats_formulas"];
        if (formulas.contains("health") && formulas["health"] == "endurance * 2") {
            state.derived_stats["health"] = state.stats["endurance"] * 2;
            state.current_health = state.derived_stats["health"];
            state.max_health = state.derived_stats["health"];
        }
    }

    // Устанавливаем начальную сцену
    state.current_scene = "scene1";

    // Очищаем флаги и инвентарь
    state.flags.clear();
    state.inventory.clear();
    state.visited_scenes.clear();

    // Загрузка начального инвентаря - ДОБАВЛЕНО ПОДТВЕРЖДЕНИЕ ЗАГРУЗКИ
    if (char_base.contains("starting_inventory")) {
        state.inventory = char_base["starting_inventory"].get<std::vector<std::string>>();
        std::cout << "DEBUG: Loaded " << state.inventory.size() << " starting items\n";
    }
    else {
        std::cerr << "WARNING: No starting inventory found in character_base.json\n";
    }
}