#include "CharacterCreator.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <algorithm>

CharacterCreator::CharacterCreator(DataManager& data, GameState& state)
    : data_(data), state_(state), first_display_(true) {
}

void CharacterCreator::Process() {
    // Всегда используем CharacterLoader для инициализации характеристик
    CharacterLoader::LoadFromJson(state_, data_.Get("character_base"));

    // Первоначальное отображение с полными описаниями
    DisplayStats(true);
    first_display_ = false;

    while (state_.stat_points > 0) {
        DistributePoints();
    }

    FinalizeCreation();
    DisplayInventory();
}


void CharacterCreator::DisplayStats(bool full_descriptions) {
    const auto& char_base = data_.Get("character_base");
    const auto& descriptions = char_base["descriptions"];
    const auto& display_names = char_base["display_names"];
    const auto& name_lengths = char_base["name_lengths"];

    // Рассчитываем максимальную длину названия
    int max_name_length = CalculateMaxNameLength();

    if (first_display_) {
        std::cout << "\n============== ХАРАКТЕРИСТИКИ ПЕРСОНАЖА ==============\n";
    }
    else {
        std::cout << "\n================================================\n";
    }

    // Выводим базовые характеристики
    for (const auto& stat : { "melee", "ranged", "strength", "endurance", "dexterity", "intelligence" }) {
        if (ShouldDisplayStat(stat)) {
            DisplaySingleStat(stat, display_names, name_lengths, descriptions,
                max_name_length, full_descriptions);
        }
    }

    // Разделитель между базовыми и производными характеристиками
    if (full_descriptions) {
        std::cout << "\n-----------------------------------------------\n";
    }

    // Выводим производные характеристики
    for (const auto& stat : { "health", "willpower" }) {
        if (ShouldDisplayStat(stat)) {
            DisplaySingleStat(stat, display_names, name_lengths, descriptions,
                max_name_length, full_descriptions);
        }
    }

    std::cout << "================================================\n";
}

int CharacterCreator::CalculateMaxNameLength() const {
    const auto& name_lengths = data_.Get("character_base")["name_lengths"];
    int max_length = 0;

    for (const auto& stat : { "melee", "ranged", "strength", "endurance",
                            "dexterity", "intelligence", "health", "willpower" }) {
        if (name_lengths.contains(stat)) {
            int length = name_lengths[stat].get<int>();
            if (length > max_length) max_length = length;
        }
    }

    return max_length;
}

bool CharacterCreator::ShouldDisplayStat(const std::string& stat) const {
    return state_.stats.find(stat) != state_.stats.end();
}

void CharacterCreator::DisplaySingleStat(const std::string& stat,
    const nlohmann::json& display_names,
    const nlohmann::json& name_lengths,
    const nlohmann::json& descriptions,
    int max_name_length,
    bool show_description) {
    std::string name = display_names.contains(stat) ?
        display_names[stat].get<std::string>() : stat;

    int name_len = name_lengths.contains(stat) ?
        name_lengths[stat].get<int>() : name.length();

    int padding = max_name_length - name_len;

    std::cout << "  " << name << std::string(padding, ' ')
        << ": " << std::setw(2) << state_.stats.at(stat);

    if (show_description && descriptions.contains(stat)) {
        std::string desc = descriptions[stat].get<std::string>();
        // Заменяем \n на реальные переносы строк
        size_t pos = 0;
        while ((pos = desc.find("\\n", pos)) != std::string::npos) {
            desc.replace(pos, 2, "\n    ");
            pos += 3;
        }
        std::cout << " | " << desc;
    }

    std::cout << "\n";
}

void CharacterCreator::DistributePoints() {
    const auto& display_names = data_.Get("character_base")["display_names"];
    const auto& name_lengths = data_.Get("character_base")["name_lengths"];

    // Рассчитываем максимальную длину названия
    int max_name_length = 0;
    for (const auto& stat : { "melee", "ranged", "strength", "endurance",
                            "dexterity", "intelligence" }) {
        if (name_lengths.contains(stat)) {
            int length = name_lengths[stat].get<int>();
            if (length > max_name_length) max_name_length = length;
        }
    }

    // Выводим меню выбора
    std::cout << "\nВЫБЕРИТЕ ХАРАКТЕРИСТИКУ ДЛЯ УЛУЧШЕНИЯ:\n";
    int index = 1;
    for (const auto& stat : { "melee", "ranged", "strength", "endurance", "dexterity", "intelligence" }) {
        if (display_names.contains(stat) && name_lengths.contains(stat)) {
            std::string name = display_names[stat].get<std::string>();
            int name_len = name_lengths[stat].get<int>();
            int padding = max_name_length - name_len;

            std::cout << "  " << index++ << ". " << name << std::string(padding, ' ')
                << " [" << state_.stats[stat] << "]\n";
        }
    }
    std::cout << "  " << index << ". Завершить распределение\n";
    std::cout << "-----------------------------------------------\n";

    // Получаем выбор характеристики
    int choice = rpg_utils::Input::GetInt(1, index);
    if (choice == index) {
        state_.stat_points = 0;
        return;
    }

    // Получаем количество очков для вложения
    const std::string stat = GetStatByIndex(choice);
    std::cout << "Сколько очков вложить? (доступно: " << state_.stat_points << "): ";
    int points = rpg_utils::Input::GetInt(1, state_.stat_points);

    // Применяем изменения
    if (ApplyPoints(stat, points)) {
        std::string name = display_names.contains(stat) ?
            display_names[stat].get<std::string>() : stat;

        std::cout << "\n  >> Характеристика '" << name << "' улучшена на "
            << points << " пунктов! Теперь: " << state_.stats[stat] << "\n";
    }
}

std::string CharacterCreator::GetStatByIndex(int index) {
    const std::vector<std::string> stats = {
        "melee", "ranged", "strength", "endurance", "dexterity", "intelligence"
    };
    return stats[index - 1];
}

bool CharacterCreator::ApplyPoints(const std::string& stat, int points) {
    if (points <= 0 || points > state_.stat_points) return false;

    // Увеличиваем выбранную характеристику
    state_.stats[stat] += points;
    state_.stat_points -= points;

    // Пересчитываем производные характеристики
    const auto& char_base = data_.Get("character_base");
    if (char_base.contains("derived_stats")) {
        const auto& derived_stats = char_base["derived_stats"];

        for (const auto& [derived_stat, formula] : derived_stats.items()) {
            try {
                std::string formula_str = formula.get<std::string>();
                auto tokens = rpg_utils::Split(rpg_utils::Trim(formula_str), ' ');

                if (tokens.size() == 3 && tokens[1] == "*") {
                    if (tokens[0] == stat) {
                        int multiplier = std::stoi(tokens[2]);
                        state_.stats[derived_stat] = state_.stats[stat] * multiplier;
                    }
                }
            }
            catch (...) {
                // Игнорируем ошибки расчета
            }
        }
    }

    return true;
}

void CharacterCreator::DisplayInventory() {
    const auto& game_state = data_.Get("game_state");

    // Проверяем наличие инвентаря
    if (!game_state.contains("inventory") || !game_state["inventory"].is_array()) {
        return;
    }

    // Получаем базовый инвентарь из game_state.json
    std::vector<std::string> base_inventory;
    for (const auto& item : game_state["inventory"]) {
        base_inventory.push_back(item.get<std::string>());
    }

    // Получаем данные всех предметов
    const auto& all_items = data_.Get("items");

    // Собираем информацию для вывода
    std::vector<std::string> item_names;
    std::vector<int> item_name_lengths;
    std::vector<std::string> item_descriptions;

    // Рассчитываем максимальную длину названия
    int max_name_length = 0;

    for (const auto& item_id : base_inventory) {
        if (all_items.contains(item_id)) {
            const auto& item = all_items[item_id];

            // Получаем название
            std::string name = item.contains("name") ?
                item["name"].get<std::string>() : item_id;
            item_names.push_back(name);

            // Получаем длину названия
            int name_length = item.contains("name_length") ?
                item["name_length"].get<int>() : name.length();
            item_name_lengths.push_back(name_length);

            // Обновляем максимальную длину
            if (name_length > max_name_length) {
                max_name_length = name_length;
            }

            // Получаем описание
            std::string desc = item.contains("description") ?
                item["description"].get<std::string>() : "(нет описания)";
            item_descriptions.push_back(desc);
        }
    }

    // Выводим инвентарь
    std::cout << "\n\n=== ВАШ СТАРТОВЫЙ ИНВЕНТАРЬ ===\n";

    if (item_names.empty()) {
        std::cout << "  Ваш инвентарь пуст\n";
    }
    else {
        for (size_t i = 0; i < item_names.size(); i++) {
            // Рассчитываем отступ для выравнивания
            int padding = max_name_length - item_name_lengths[i];

            std::cout << "  • " << item_names[i]
                << std::string(padding, ' ')
                    << " - " << item_descriptions[i] << "\n";
        }
    }

    std::cout << "===========================================\n";

    // Сохраняем инвентарь в текущее состояние
    state_.inventory = base_inventory;
}


void CharacterCreator::FinalizeCreation() {
    state_.flags["character_created"] = true;
    std::cout << "\n================================================\n";
    std::cout << "         СОЗДАНИЕ ПЕРСОНАЖА ЗАВЕРШЕНО!\n";
    std::cout << "================================================\n";

    // Выводим финальные характеристики
    DisplayStats(false);
}