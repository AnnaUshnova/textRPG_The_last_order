#include "CharacterCreator.h"
#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>

CharacterCreator::CharacterCreator(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void CharacterCreator::Process() {
    ShowWelcomeScreen();
    DistributePoints();
    FinalizeCharacter();
    ShowSummary();
}

void CharacterCreator::ShowWelcomeScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    std::cout << "===================================\n";
    std::cout << "|   СОЗДАНИЕ ПЕРСОНАЖА           |\n";
    std::cout << "===================================\n";
    std::cout << "Вам доступно " << state_.stat_points << " очков для распределения.\n";
    std::cout << "Распределите их между основными характеристиками.\n\n";
}

CharacterCreator::CharacterConfig CharacterCreator::LoadConfig() const {
    CharacterConfig config;

    try {
        const auto& char_data = data_.Get("character_base");

        // Загрузка списка основных характеристик
        if (char_data.contains("core_stats") && char_data["core_stats"].is_array()) {
            config.core_stats = char_data["core_stats"].get<std::vector<std::string>>();
        }
        else {
            // Значения по умолчанию
            config.core_stats = { "strength", "dexterity", "endurance", "intelligence", "melee", "ranged" };
        }

        // Загрузка отображаемых имен
        if (char_data.contains("display_names")) {
            config.display_names = char_data["display_names"]
                .get<std::unordered_map<std::string, std::string>>();
        }

        // Загрузка описаний
        if (char_data.contains("descriptions")) {
            config.descriptions = char_data["descriptions"]
                .get<std::unordered_map<std::string, std::string>>();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки конфигурации: " << e.what() << "\n";

        // Резервные значения
        config.core_stats = { "strength", "dexterity", "endurance", "intelligence", "melee", "ranged" };
        config.display_names = {
            {"strength", "Сила"},
            {"dexterity", "Ловкость"},
            {"endurance", "Выносливость"},
            {"intelligence", "Интеллект"},
            {"melee", "Ближний бой"},
            {"ranged", "Дальний бой"}
        };
    }

    return config;
}

void CharacterCreator::DisplayStats(bool show_descriptions) {
    auto config = LoadConfig();

    std::cout << "Основные характеристики:\n";
    for (size_t i = 0; i < config.core_stats.size(); ++i) {
        const std::string& stat_key = config.core_stats[i];

        // Получаем отображаемое имя
        std::string display_name = stat_key;
        if (config.display_names.find(stat_key) != config.display_names.end()) {
            display_name = config.display_names.at(stat_key);
        }

        // Получаем значение характеристики
        int value = 0;
        if (state_.stats.find(stat_key) != state_.stats.end()) {
            value = state_.stats.at(stat_key);
        }

        std::cout << "  " << i + 1 << ". " << display_name << ": " << value;

        // Добавляем описание при необходимости
        if (show_descriptions && config.descriptions.find(stat_key) != config.descriptions.end()) {
            std::cout << " - " << config.descriptions.at(stat_key);
        }
        std::cout << "\n";
    }

    // Показываем производные характеристики
    if (!state_.derived_stats.empty()) {
        std::cout << "\nПроизводные характеристики:\n";
        for (const auto& [stat, val] : state_.derived_stats) {
            std::string display_name = stat;
            if (config.display_names.find(stat) != config.display_names.end()) {
                display_name = config.display_names.at(stat);
            }

            // Форматируем вывод чисел с плавающей точкой
            if (std::abs(val - std::round(val)) < 0.001) {
                std::cout << "  " << display_name << ": " << static_cast<int>(val) << "\n";
            }
            else {
                std::cout << "  " << display_name << ": " << val << "\n";
            }
        }
    }
}

int CharacterCreator::GetStatIndexInput() const {
    auto config = LoadConfig();
    int min = 0;
    int max = static_cast<int>(config.core_stats.size());

    while (true) {
        std::cout << "Ваш выбор (0-" << max << "): ";
        std::string input;
        std::getline(std::cin, input);

        try {
            int choice = std::stoi(input);
            if (choice >= min && choice <= max) {
                return choice;
            }
            std::cout << "Пожалуйста, введите число от " << min << " до " << max << ".\n";
        }
        catch (...) {
            std::cout << "Неверный ввод. Пожалуйста, введите число.\n";
        }
    }
}

void CharacterCreator::ApplyPointToStat(const std::string& stat) {
    // Инициализируем характеристику, если ее еще нет
    if (state_.stats.find(stat) == state_.stats.end()) {
        state_.stats[stat] = 0;
    }

    state_.stats[stat]++;
}

void CharacterCreator::CalculateDerivedStats() {
    const auto& char_data = data_.Get("character_base");
    rpg_utils::CalculateDerivedStats(state_, char_data);
}

void CharacterCreator::DistributePoints() {
    auto config = LoadConfig();

    while (state_.stat_points > 0) {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        ShowWelcomeScreen();
        DisplayStats(true);

        std::cout << "\nВыберите характеристику для улучшения (0 - завершить): ";
        int choice = GetStatIndexInput();

        if (choice == 0) {
            break;
        }

        if (choice > 0 && choice <= static_cast<int>(config.core_stats.size())) {
            const std::string& stat = config.core_stats[choice - 1];
            ApplyPointToStat(stat);
            state_.stat_points--;

            // Пересчет производных характеристик
            CalculateDerivedStats();

            std::cout << "\nХарактеристика улучшена! Осталось очков: " << state_.stat_points << "\n";
            std::cout << "Нажмите Enter чтобы продолжить...";
            rpg_utils::Input::GetLine();
        }
    }
}

void CharacterCreator::FinalizeCharacter() {
    // Убедимся, что все характеристики инициализированы
    auto config = LoadConfig();
    for (const auto& stat : config.core_stats) {
        if (state_.stats.find(stat) == state_.stats.end()) {
            state_.stats[stat] = 0;
        }
    }

    // Установим начальное здоровье
    if (state_.derived_stats.find("health") != state_.derived_stats.end()) {
        state_.current_health = static_cast<int>(state_.derived_stats["health"]);
    }

    // Сохраняем флаг создания персонажа
    state_.flags["character_created"] = true;
}

void CharacterCreator::ShowSummary() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    std::cout << "===================================\n";
    std::cout << "|   ПЕРСОНАЖ СОЗДАН УСПЕШНО!      |\n";
    std::cout << "===================================\n";
    std::cout << "Итоговые характеристики:\n\n";

    DisplayStats(false);

    std::cout << "\nНажмите Enter чтобы начать игру...";
    rpg_utils::Input::GetLine();
}