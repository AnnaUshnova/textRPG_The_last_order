#include "DataManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

void DataManager::LoadAll(const std::string& data_dir) {
    try {
        // Проход по всем файлам в указанной директории
        for (const auto& entry : fs::directory_iterator(data_dir)) {
            // Загрузка только JSON-файлов
            if (entry.path().extension() == ".json") {
                std::string type = entry.path().stem().string();
                LoadJsonFile(entry.path().string(), type);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки данных: " << e.what() << std::endl;
    }
}

void DataManager::LoadJsonFile(const std::string& filename,
    const std::string& type) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл: " + filename);
        }

        nlohmann::json data;
        file >> data;

        // Если JSON содержит раздел с именем файла, используем его
        if (data.contains(type)) {
            data_sets_[type] = data[type];
        }
        else {
            // Иначе используем весь JSON-объект
            data_sets_[type] = data;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки данных типа " << type << ": "
            << e.what() << std::endl;
    }
}

const nlohmann::json& DataManager::Get(const std::string& type) const {
    // Пустой JSON-объект для возврата по умолчанию
    static const nlohmann::json empty = nlohmann::json::object();

    // Поиск запрошенного типа данных
    auto it = data_sets_.find(type);
    return (it != data_sets_.end()) ? it->second : empty;
}

nlohmann::json& DataManager::GetMutable(const std::string& type) {
    // Возврат изменяемой ссылки на JSON-данные
    return data_sets_[type];
}

void DataManager::SaveGameState(const std::string& filename,
    const GameState& state) {
    nlohmann::json j;
    j["unlocked_endings"] = nlohmann::json::array();

    // Сохранение списка открытых концовок
    for (const auto& ending : state.unlocked_endings) {
        j["unlocked_endings"].push_back(ending);
    }

    // Запись данных в файл
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);  // Форматированный вывод с отступами
    }
}

void DataManager::LoadGameState(const std::string& filename, GameState& state) {
    std::ifstream file(filename);
    if (!file.is_open()) return;  // Файл не существует - пропускаем загрузку

    try {
        nlohmann::json j;
        file >> j;

        // Загрузка списка открытых концовок
        if (j.contains("unlocked_endings")) {
            state.unlocked_endings.clear();
            for (const auto& item : j["unlocked_endings"]) {
                state.unlocked_endings.insert(item.get<std::string>());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки сохранения: " << e.what() << "\n";
    }
}

void DataManager::ResetGameState(GameState& state) {
    // Сохранение открытых концовок перед сбросом
    std::unordered_set<std::string> saved_endings = state.unlocked_endings;

    // Полный сброс состояния игры
    state = GameState();
    state.unlocked_endings = saved_endings;

    // Загрузка базовых характеристик персонажа
    const auto& char_base = Get("character_base");
    state.stats = char_base["base_stats"]
        .get<std::unordered_map<std::string, int>>();
    state.stat_points = char_base["points_to_distribute"].get<int>();

    // Расчет производных характеристик
    if (char_base.contains("derived_stats_formulas")) {
        const auto& formulas = char_base["derived_stats_formulas"];
        if (formulas.contains("health") && formulas["health"] == "endurance * 2") {
            state.derived_stats["health"] = state.stats["endurance"] * 2;
            state.current_health = state.derived_stats["health"];
            state.max_health = state.derived_stats["health"];
        }
    }

    // Начальные значения игрового состояния
    state.current_scene = "scene1";
    state.flags.clear();
    state.inventory.clear();
    state.visited_scenes.clear();

    // Загрузка стартового инвентаря
    if (char_base.contains("starting_inventory")) {
        for (const auto& item : char_base["starting_inventory"]) {
            if (item.is_string()) {
                state.inventory[item.get<std::string>()]++;
            }
            else if (item.is_object()) {
                std::string item_id = item["id"].get<std::string>();
                int count = item.value("count", 1);
                state.inventory[item_id] += count;
            }
        }
    }
}

nlohmann::json DataManager::GetItem(const std::string& item_id) const {
    // Получение данных о конкретном предмете
    const auto& items = Get("items");
    return items.contains(item_id) ? items[item_id] : nlohmann::json();
}