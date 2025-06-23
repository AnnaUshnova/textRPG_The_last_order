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

// Новая функция: Сохранение состояния игры
void DataManager::SaveGameState(const std::string& filename, const GameState& state) {
    nlohmann::json j;

    // Основные параметры
    j["current_scene"] = state.current_scene;
    j["current_health"] = state.current_health;
    j["stat_points"] = state.stat_points;
    j["quit_game"] = state.quit_game;

    // Словари
    j["stats"] = state.stats;
    j["derived_stats"] = state.derived_stats;
    j["flags"] = state.flags;

    // Списки
    j["inventory"] = state.inventory;
    j["unlocked_endings"] = state.unlocked_endings;

    // Состояние боя
    j["combat"]["enemy_id"] = state.combat.enemy_id;
    j["combat"]["enemy_health"] = state.combat.enemy_health;
    j["combat"]["current_phase"] = state.combat.current_phase;
    j["combat"]["player_turn"] = state.combat.player_turn;

    // Запись в файл
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);  // Форматирование с отступами
        std::cout << "Игра сохранена в: " << filename << "\n";
    }
    else {
        std::cerr << "Ошибка: не удалось открыть файл для записи: " << filename << "\n";
    }
}

// Новая функция: Загрузка состояния игры
void DataManager::LoadGameState(const std::string& filename, GameState& state) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: файл сохранения не найден: " << filename << "\n";
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        // Основные параметры
        state.current_scene = j.value("current_scene", "main_menu");
        state.current_health = j.value("current_health", 0);
        state.stat_points = j.value("stat_points", 0);
        state.quit_game = j.value("quit_game", false);

        // Словари
        if (j.contains("stats")) state.stats = j["stats"].get<std::unordered_map<std::string, int>>();
        if (j.contains("derived_stats")) state.derived_stats = j["derived_stats"].get<std::unordered_map<std::string, int>>();
        if (j.contains("flags")) state.flags = j["flags"].get<std::unordered_map<std::string, bool>>();

        // Списки
        if (j.contains("inventory")) state.inventory = j["inventory"].get<std::vector<std::string>>();
        if (j.contains("unlocked_endings")) {
            for (const auto& ending : j["unlocked_endings"]) {
                state.unlocked_endings.insert(ending.get<std::string>());
            }
        }

        // Состояние боя
        if (j.contains("combat")) {
            state.combat.enemy_id = j["combat"].value("enemy_id", "");
            state.combat.enemy_health = j["combat"].value("enemy_health", 0);
            state.combat.current_phase = j["combat"].value("current_phase", 0);
            state.combat.player_turn = j["combat"].value("player_turn", true);
        }

        std::cout << "Игра загружена из: " << filename << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки сохранения: " << e.what() << "\n";
    }
}

// Новая функция: Сброс состояния к начальному
void DataManager::ResetGameState(GameState& state, const nlohmann::json& char_base) {
    state = GameState();  // Сбрасываем состояние

    // Инициализируем базовые характеристики
    if (char_base.contains("base_stats")) {
        state.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
    }

    state.stat_points = char_base.value("points_to_distribute", 10);
    state.current_scene = char_base.value("start_scene", "main_menu");

    std::cout << "Состояние игры сброшено к начальному\n";
}