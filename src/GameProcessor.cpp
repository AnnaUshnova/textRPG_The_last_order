#include "GameProcessor.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <iomanip>

GameProcessor::GameProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void GameProcessor::ShowEnding(const std::string& ending_id) {
    const auto& endings = data_.Get("endings");
    if (!endings.contains(ending_id)) {
        std::cerr << "Концовка не найдена: " << ending_id << std::endl;
        state_.current_scene = "main_menu";
        return;
    }

    const auto& ending = endings[ending_id];

    // Оформление концовки
    std::cout << "\n\n";
    std::cout << "========================================\n";
    std::cout << "           ИГРА ОКОНЧЕНА!               \n";
    std::cout << "========================================\n\n";

    if (ending.contains("title")) {
        std::cout << "  » " << ending["title"].get<std::string>() << " «\n\n";
    }

    if (ending.contains("text")) {
        std::cout << ending["text"].get<std::string>() << "\n\n";
    }

    if (ending.contains("achievement")) {
        std::cout << "----------------------------------------\n";
        std::cout << "Достижение: " << ending["achievement"].get<std::string>() << "\n";
    }

    std::cout << "========================================\n\n";

    // Добавляем концовку
    if (state_.unlocked_endings.find(ending_id) == state_.unlocked_endings.end()) {
        state_.unlocked_endings.insert(ending_id);
        std::cout << "Концовка добавлена: " << ending_id << "\n";

        // Немедленно сохраняем игру
        data_.SaveGameState("save.json", state_);
    }
    else {
        std::cout << "Концовка уже была получена ранее\n";
    }

    // Пауза для чтения
    std::cout << "Нажмите Enter, чтобы продолжить...";
    rpg_utils::Input::GetLine();

    // Добавляем концовку в коллекцию
    state_.unlocked_endings.insert(ending_id);

    if (state_.unlocked_endings.find(ending_id) == state_.unlocked_endings.end()) {
        state_.unlocked_endings.insert(ending_id);
        data_.SaveGameState("save.json", state_); // Сохраняем концовку
    }

    state_.current_scene = "main_menu";
}

void GameProcessor::ShowEndingCollection() {
    const auto& endings = data_.Get("endings");

    if (state_.unlocked_endings.empty()) {
        std::cout << "\nВы пока не получили ни одной концовки!\n";
        std::cout << "Пройдите игру, чтобы открыть достижения.\n";
    }
    else {
        std::cout << "\n===== ВАШИ ДОСТИЖЕНИЯ =====\n";
        std::cout << "Получено: " << state_.unlocked_endings.size()
            << " из " << endings.size() << " концовок\n\n";


        int index = 1;
        std::vector<std::string> sorted_endings(
            state_.unlocked_endings.begin(),
            state_.unlocked_endings.end()
        );
        std::sort(sorted_endings.begin(), sorted_endings.end());

        for (const auto& ending_id : sorted_endings) {
            if (endings.contains(ending_id)) {
                const auto& ending = endings[ending_id];
                std::cout << index++ << ". " << ending["title"].get<std::string>();

                if (ending.contains("achievement")) {
                    std::cout << " («" << ending["achievement"].get<std::string>() << "»)";
                }

                std::cout << "\n";
            }
        }
    }



    std::cout << "\nНажмите Enter, чтобы вернуться...";
    rpg_utils::Input::GetLine();
}

void GameProcessor::CalculateDerivedStats() {
    const auto& formulas = data_.Get("character_base")["derived_stats_formulas"];
    for (const auto& [stat, formula] : formulas.items()) {
        if (formula.get<std::string>() == "endurance * 2") {
            state_.derived_stats[stat] = state_.stats.at("endurance") * 2;
        }
        else if (formula.get<std::string>() == "intelligence * 2") {
            state_.derived_stats[stat] = state_.stats.at("intelligence") * 2;
        }
    }

    // Добавьте вывод для отладки
    std::cout << "Рассчитанные производные характеристики:\n";
    for (const auto& [stat, value] : state_.derived_stats) {
        std::cout << stat << ": " << value << "\n";
    }
}   

void GameProcessor::DisplayEnding(const nlohmann::json& ending) {
    std::cout << "\n\n===== " << ending["title"].get<std::string>() << " =====\n\n";
    std::cout << ending["text"].get<std::string>() << "\n\n";

    if (ending.contains("achievement")) {
        std::cout << "Открыто достижение: "
            << ending["achievement"].get<std::string>() << "\n";
    }

    std::cout << "\nИгра окончена\n";
}

void GameProcessor::ProcessCheck(const nlohmann::json& check_data) {
    // Проверяем тип проверки
    if (!check_data.contains("type")) {
        std::cerr << "Ошибка: проверка не содержит типа\n";
        state_.current_scene = "main_menu";
        return;
    }

    const std::string stat = check_data["type"].get<std::string>();
    const int difficulty = check_data.value("difficulty", 0);
    const int base_value = state_.stats.at(stat);

    // Бросок кубиков
    auto roll = rpg_utils::RollDiceWithModifiers(base_value, difficulty);
    std::cout << "\nПроверка " << stat << " (" << base_value << "): "
        << roll.total_roll << " [Сложность: " << difficulty << "]\n";

    // Определяем результат броска
    std::string result_key;
    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess:
        result_key = "critical_success";
        std::cout << "Критический успех!\n";
        break;
    case rpg_utils::RollResultType::kCriticalFail:
        result_key = "critical_fail";
        std::cout << "Критическая неудача!\n";
        break;
    case rpg_utils::RollResultType::kSuccess:
        result_key = "success";
        std::cout << "Успех!\n";
        break;
    case rpg_utils::RollResultType::kFail:
        result_key = "fail";
        std::cout << "Неудача!\n";
        break;
    }

    // Логирование для отладки
    std::cout << "--- DEBUG: Check result: " << result_key << " ---\n";

    // Определяем следующую сцену
    if (check_data.contains("results") && check_data["results"].contains(result_key)) {
        state_.current_scene = check_data["results"][result_key].get<std::string>();
        std::cout << "--- DEBUG: Next scene: " << state_.current_scene << " ---\n";
    }
    else if (check_data.contains(result_key)) {
        state_.current_scene = check_data[result_key].get<std::string>();
        std::cout << "--- DEBUG: Next scene: " << state_.current_scene << " ---\n";
    }
    else if (result_key == "critical_success" && check_data.contains("success")) {
        state_.current_scene = check_data["success"].get<std::string>();
        std::cout << "--- DEBUG: Using success as fallback for critical_success ---\n";
    }
    else if (result_key == "critical_fail" && check_data.contains("fail")) {
        state_.current_scene = check_data["fail"].get<std::string>();
        std::cout << "--- DEBUG: Using fail as fallback for critical_fail ---\n";
    }
    else if (check_data.contains("next_scene")) {
        state_.current_scene = check_data["next_scene"].get<std::string>();
        std::cout << "--- DEBUG: Using next_scene fallback ---\n";
    }
    else {
        std::cerr << "ERROR: No next scene defined for check result: " << result_key << "\n";
        state_.current_scene = "main_menu";
    }
}

bool GameProcessor::EvaluateCondition(const std::string& condition) {
    // Простые условия с флагами
    if (condition.find("flags.") == 0) {
        std::string flag_name = condition.substr(6);
        bool flag_value = state_.flags.count(flag_name) ? state_.flags[flag_name] : false;
        return flag_value;
    }

    // Проверка наличия предмета с количеством
    if (condition.find("has_item.") == 0) {
        size_t dot_pos = condition.find('.', 9);
        if (dot_pos != std::string::npos) {
            std::string item_id = condition.substr(9, dot_pos - 9);
            int required_count = std::stoi(condition.substr(dot_pos + 1));
            return HasItem(item_id, required_count);
        }
        else {
            std::string item_id = condition.substr(9);
            return HasItem(item_id, 1);
        }
    }

    // Проверка значения характеристики
    size_t op_pos = condition.find_first_of("<>=");
    if (op_pos != std::string::npos) {
        std::string stat_name = condition.substr(0, op_pos);
        char op = condition[op_pos];
        int value = std::stoi(condition.substr(op_pos + 1));

        if (state_.stats.count(stat_name)) {
            int stat_value = state_.stats[stat_name];
            switch (op) {
            case '>': return stat_value > value;
            case '<': return stat_value < value;
            case '=': return stat_value == value;
            case '!': return stat_value != value; // Для !=
            }
        }
    }

    // Обработка комбинированных условий
    if (condition.find("&&") != std::string::npos) {
        std::vector<std::string> sub_conds = rpg_utils::Split(condition, '&');
        for (const auto& cond : sub_conds) {
            if (!EvaluateCondition(cond)) return false;
        }
        return true;
    }

    if (condition.find("||") != std::string::npos) {
        std::vector<std::string> sub_conds = rpg_utils::Split(condition, '|');
        for (const auto& cond : sub_conds) {
            if (EvaluateCondition(cond)) return true;
        }
        return false;
    }

    // Отрицание
    if (condition[0] == '!') {
        return !EvaluateCondition(condition.substr(1));
    }

    // По умолчанию - условие истинно
    return true;
}

void GameProcessor::HandleAutoAction(const std::string& action) {
    if (action == "start_creation") {
        InitializeCharacter();
    }
    else if (action == "show_endings") {
        ShowEndingCollection();
        // После показа коллекции остаемся в главном меню
        state_.current_scene = "main_menu";
    }
    else if (action == "quit_game") {
        state_.quit_game = true;
    }
    else if (action == "reset_state") {
        const auto& char_base = data_.Get("character_base");
        data_.ResetGameState(state_);
        std::cout << "\nИгра сброшена к начальному состоянию\n";
    }
    else if (action == "load_game") {
        data_.LoadGameState("save.json", state_);
        std::cout << "\nИгра загружена\n";
    }
    else if (action == "save_game") {
        data_.SaveGameState("save.json", state_);
        std::cout << "\nИгра сохранена\n";
    }
    else if (action.find("start_combat:") == 0) {
        std::string combat_id = action.substr(13);
        state_.current_scene = combat_id;
    }
}

void GameProcessor::ApplyGameEffects(const nlohmann::json& effects) {
    if (effects.contains("set_flags")) {
        for (const auto& [flag, value] : effects["set_flags"].items()) {
            // Упрощенная обработка значений
            if (value.is_boolean()) {
                state_.flags[flag] = value.get<bool>();
            }
            else if (value.is_number()) {
                state_.flags[flag] = (value.get<int>() != 0);
            }
            else if (value.is_string()) {
                std::string val = value.get<std::string>();
                state_.flags[flag] = (val == "true" || val == "1");
            }
        }
    }
    // ...
}

void GameProcessor::ProcessCombat(const std::string& combat_id) {
    const auto& combats = data_.Get("combats");
    if (!combats.contains(combat_id)) {
        std::cerr << "Бой не найден: " << combat_id << std::endl;
        state_.current_scene = "main_menu";
        return;
    }

    const auto& combat = combats[combat_id];
    InitializeCombat(combat, combat_id);

    // Основной боевой цикл
    while (state_.combat.enemy_health > 0 && state_.current_health > 0) {
        // Отображаем статус боя перед каждым ходом
        DisplayCombatStatus(combat);

        if (state_.combat.player_turn) {
            ProcessPlayerCombatTurn(combat);
        }
        else {
            ProcessEnemyCombatTurn(combat);
        }

        // Проверяем, не закончился ли бой после этого хода
        if (state_.combat.enemy_health <= 0 || state_.current_health <= 0) {
            break;
        }
    }

    CleanupCombat(combat);
}

void GameProcessor::InitializeCombat(const nlohmann::json& combat_data, const std::string& combat_id) {
    state_.combat = CombatState();
    state_.combat.enemy_id = combat_id;
    state_.combat.enemy_health = combat_data["health"].get<int>();
    state_.combat.max_enemy_health = combat_data["health"].get<int>();  // Инициализируем
    state_.combat.current_phase = 0;
    state_.combat.player_turn = true;
}

void GameProcessor::ProcessEnemyCombatTurn(const nlohmann::json& combat) {
    const auto& phases = combat["phases"];
    int current_phase_index = 0;

    // Находим текущую фазу боя
    for (int i = 0; i < phases.size(); ++i) {
        if (state_.combat.enemy_health <= phases[i]["health_threshold"].get<int>()) {
            current_phase_index = i;
        }
    }

    const auto& attacks = phases[current_phase_index]["attacks"];

    if (!attacks.empty()) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, attacks.size() - 1);
        const auto& attack = attacks[dist(gen)];

        std::cout << "\n=== ХОД ПРОТИВНИКА ===\n";
        std::cout << attack["description"].get<std::string>() << "\n";

        bool hit = true;
        int difficulty_modifier = 0;

        // Проверка защиты игрока
        if (attack.contains("reaction_stat")) {
            const std::string defense_stat = attack["reaction_stat"].get<std::string>();
            const int defense_value = state_.stats.at(defense_stat);

            // Получение значения атаки противника
            int attack_value = 0;
            if (attack["check_stat"].is_string()) {
                const std::string stat_name = attack["check_stat"].get<std::string>();
                attack_value = combat["stats"][stat_name].get<int>();
            }
            else if (attack["check_stat"].is_number()) {
                attack_value = attack["check_stat"].get<int>();
            }

            // Модификаторы защиты
            if (state_.flags.count("exposed") && state_.flags["exposed"]) {
                difficulty_modifier -= 2;
            }
            if (state_.flags.count("in_cover") && state_.flags["in_cover"]) {
                difficulty_modifier += 2;
            }

            // Бросок защиты
            auto defense_roll = rpg_utils::RollDiceWithModifiers(defense_value, difficulty_modifier);

            std::cout << "Ваша защита (" << defense_value;
            if (difficulty_modifier != 0) {
                std::cout << (difficulty_modifier > 0 ? "+" : "") << difficulty_modifier;
            }
            std::cout << "): " << defense_roll.total_roll << " -> ";

            // В GURPS защита успешна если бросок <= значения навыка
            if (defense_roll.result == rpg_utils::RollResultType::kCriticalSuccess ||
                defense_roll.result == rpg_utils::RollResultType::kSuccess) {
                std::cout << "УСПЕХ\n";
                hit = false;
            }
            else {
                std::cout << "НЕУДАЧА\n";
                hit = true;
            }
        }

        // Нанесение урона
        if (hit) {
            if (attack.contains("damage")) {
                int damage = rpg_utils::CalculateDamage(attack["damage"].get<std::string>());

                // Учет брони
                int armor = 0;
                if (state_.flags.count("heavy_armor") && state_.flags["heavy_armor"]) {
                    armor += 4;
                }
                else if (state_.flags.count("light_armor") && state_.flags["light_armor"]) {
                    armor += 2;
                }

                damage = std::max(0, damage - armor);
                state_.current_health = std::max(0, state_.current_health - damage);

                std::cout << "Вы получили " << damage << " урона!";
                if (armor > 0) std::cout << " (Броня поглотила " << armor << ")";
                std::cout << "\n";
            }
        }
        else {
            std::cout << "Вы успешно уклонились от атаки!\n";
        }
    }
    else {
        std::cout << "\nПротивник колеблется...\n";
    }

    state_.combat.player_turn = true;
}

void GameProcessor::DisplayCombatStatus(const nlohmann::json& combat_data) {
    // Получаем имя противника
    std::string enemy_name = combat_data["enemy"].get<std::string>();

    // Получаем имена характеристик
    const auto& display_names = data_.Get("character_base")["display_names"];
    std::string health_name = display_names.value("health", "Здоровье");

    // Выводим статус боя
    std::cout << "\n===== БОЙ =====\n";
    std::cout << "Противник: " << enemy_name << "\n";
    std::cout << health_name << " противника: " << state_.combat.enemy_health
        << "/" << state_.combat.max_enemy_health << "\n";
    std::cout << "Ваше " << health_name << ": " << state_.current_health
        << "/" << state_.max_health << "\n";

    // Выводим окружение, если есть
    if (combat_data.contains("environment")) {
        std::cout << "\nОкружение:\n";
        for (const auto& env : combat_data["environment"]) {
            std::cout << "- " << env["name"].get<std::string>() << ": "
                << env["effect"].get<std::string>() << "\n";
        }
    }

    std::cout << "================\n";
}

void GameProcessor::ApplyCombatResults(const nlohmann::json& results) {
    // Применяем общие эффекты
    if (!results.is_null()) {
        ApplyGameEffects(results);
    }

    // Обработка концовки
    if (results.contains("ending")) {
        ShowEnding(results["ending"].get<std::string>());
        return;
    }

    // Обработка обычных переходов
    if (results.contains("next_scene")) {
        state_.current_scene = results["next_scene"].get<std::string>();
    }
    else {
        state_.current_scene = "main_menu";
    }

    // Сбрасываем боевое состояние
    state_.combat = CombatState();
}

void GameProcessor::InitializeNewGame() {
    const auto& char_base = data_.Get("character_base");

    // Полный сброс состояния
    state_ = GameState();

    // Загрузка базовых характеристик
    state_.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
    state_.stat_points = char_base["points_to_distribute"].get<int>();

    // Расчет производных характеристик
    CalculateDerivedStats();
    state_.current_health = state_.derived_stats["health"];
    state_.max_health = state_.derived_stats["health"];

    // Начальная сцена
    state_.current_scene = "scene1";

    std::cout << "\nНовая игра начата!\n";
}

void GameProcessor::StartNewGame() {
    // Сброс состояния
    data_.ResetGameState(state_);

    // Начинаем с первой сцены
    state_.current_scene = "scene1";

    std::cout << "\n===================================\n";
    std::cout << "        НОВАЯ ИГРА НАЧАТА!        \n";
    std::cout << "===================================\n\n";
}

void GameProcessor::FullReset() {
    // Полностью сбрасываем состояние
    state_ = GameState();

    // Удаляем файл сохранения
    std::remove("save.json");

    std::cout << "\n===================================\n";
    std::cout << "     ПРОГРЕСС ПОЛНОСТЬЮ СБРОШЕН!    \n";
    std::cout << "===================================\n\n";

    // Возврат в главное меню
    state_.current_scene = "main_menu";
}

void GameProcessor::UseItemOutsideCombat() {
    const auto& items_data = data_.Get("items");
    std::vector<std::pair<std::string, int>> usable_items;

    // Собираем предметы, которые можно использовать
    for (const auto& [item_id, count] : state_.inventory) {
        if (items_data.contains(item_id)) {
            const auto& item = items_data[item_id];
            // Исправлено условие: используем логическое И (&&) вместо побитового И (&)
            if (item.contains("effects") &&
                item.value("type", "") == "consumable") {
                usable_items.push_back({ item_id, count });
            }
        }
    }

    if (usable_items.empty()) {
        std::cout << "Нет предметов для использования\n";
        return;
    }

    std::cout << "\nИспользовать предмет:\n";
    for (size_t i = 0; i < usable_items.size(); i++) {
        const auto& item = items_data[usable_items[i].first];
        std::cout << (i + 1) << ". " << item["name"].get<std::string>();
        if (usable_items[i].second > 1) {
            std::cout << " (x" << usable_items[i].second << ")";
        }
        std::cout << "\n";
    }
    std::cout << "0. Отмена\n";

    // Исправленный вызов GetInt с двумя аргументами
    int choice = rpg_utils::Input::GetInt(0, static_cast<int>(usable_items.size()));
    if (choice > 0) {
        const std::string& item_id = usable_items[choice - 1].first;
        const auto& item = items_data[item_id];

        // Применяем эффекты
        if (item.contains("effects")) {
            ApplyGameEffects(item["effects"]);
        }

        // Удаляем один предмет
        RemoveItemFromInventory(item_id, 1);
        std::cout << "Предмет использован\n";
    }
}

void GameProcessor::InitializeCharacter() {
    const auto& char_base = data_.Get("character_base");

    // Проверка наличия необходимых полей
    if (!char_base.contains("base_stats") ||
        !char_base.contains("points_to_distribute") ||
        !char_base.contains("display_names") ||
        !char_base.contains("descriptions"))
    {
        std::cerr << "Ошибка: неполные данные для создания персонажа\n";
        state_.current_scene = "main_menu";
        return;
    }

    // Загрузка базовых характеристик из JSON
    state_.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
    state_.stat_points = char_base["points_to_distribute"].get<int>();

    const auto& display_names = char_base["display_names"].get<std::unordered_map<std::string, std::string>>();
    const auto& descriptions = char_base["descriptions"].get<std::unordered_map<std::string, std::string>>();

    // Список характеристик для распределения
    const std::vector<std::string> core_stats = {
        "strength", "dexterity", "endurance", "intelligence", "melee", "ranged"
    };

    // Вывод описаний характеристик
    std::cout << "\n===== ОПИСАНИЕ ХАРАКТЕРИСТИК =====\n";
    for (const auto& stat : core_stats) {
        if (descriptions.find(stat) == descriptions.end()) {
            std::cerr << "Предупреждение: отсутствует описание для '" << stat << "'\n";
            continue;
        }

        std::cout << display_names.at(stat) << ":\n";
        std::string desc = descriptions.at(stat);

        // Замена \n на настоящие переносы строк
        size_t pos = 0;
        while ((pos = desc.find("\\n", pos)) != std::string::npos) {
            desc.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << desc << "\n\n";
    }

    // Распределение очков характеристик
    while (state_.stat_points > 0) {
        std::cout << "\n===== СОЗДАНИЕ ПЕРСОНАЖА =====\n";
        std::cout << "Осталось очков: " << state_.stat_points << "\n\n";

        // Отображение текущих характеристик
        for (size_t i = 0; i < core_stats.size(); i++) {
            const std::string& stat = core_stats[i];
            std::cout << (i + 1) << ". " << display_names.at(stat)
                << ": " << state_.stats[stat] << "\n";
        }

        // Выбор характеристики
        std::cout << "\nВыберите характеристику (1-" << core_stats.size() << "): ";
        int stat_index = rpg_utils::Input::GetInt(1, core_stats.size()) - 1;
        const std::string& chosen_stat = core_stats[stat_index];

        std::cout << "Сколько очков добавить (1-" << state_.stat_points << "): ";
        int points = rpg_utils::Input::GetInt(1, state_.stat_points);

        state_.stats[chosen_stat] += points;
        state_.stat_points -= points;
    }

    // Расчет производных характеристик
    CalculateDerivedStats();

    // Установка здоровья
    state_.current_health = state_.derived_stats["health"];
    state_.max_health = state_.derived_stats["health"];

    std::cout << "\nПерсонаж успешно создан!\n";
    std::cout << "Здоровье: " << state_.current_health << "/" << state_.max_health << "\n\n";

    // Загрузка начального инвентаря - ИСПРАВЛЕННАЯ ЧАСТЬ
    if (char_base.contains("starting_inventory")) {
        for (const auto& item : char_base["starting_inventory"]) {
            // Проверяем тип элемента перед обработкой
            if (item.is_object() && item.contains("id")) {
                std::string item_id = item["id"].get<std::string>();
                int count = item.value("count", 1);

                // Используем метод добавления с проверкой
                if (!item_id.empty()) {
                    AddItemToInventory(item_id, count);
                }
            }
        }
    }

    // Показ инвентаря после создания персонажа
    ShowInventory();

    // Переход к игровому процессу
    state_.current_scene = "scene7";

    std::cout << "Персонаж создан. Прогресс не сохраняется, сохраняются только концовки.\n";
}

void GameProcessor::ProcessScene(const std::string& scene_id) {
    // Отладочная информация
    std::cout << "\n[DEBUG] Processing scene: " << scene_id << "\n";
    std::cout << "[DEBUG] Inventory size: " << state_.inventory.size() << "\n";

    // Обработка боевых сцен
    if (scene_id.find("combat_") == 0) {
        ProcessCombat(scene_id);
        return;
    }

    // Обработка концовок
    if (scene_id.find("ending") == 0) {
        ShowEnding(scene_id);
        return;
    }

    // Поиск сцены в различных источниках данных
    const std::vector<std::string> sources = { "scenes", "checks", "endings" };
    nlohmann::json scene_data;
    std::string source_name = "unknown";
    bool scene_found = false;

    for (const auto& source : sources) {
        const auto& data = data_.Get(source);
        if (!data.is_null() && data.is_object() && data.contains(scene_id)) {
            scene_data = data[scene_id];
            source_name = source;
            scene_found = true;
            break;
        }
    }

    // Обработка случая, когда сцена не найдена
    if (!scene_found) {
        std::cerr << "ERROR: Scene not found: " << scene_id << "\n";
        state_.current_scene = "main_menu";
        return;
    }

    std::cout << "[DEBUG] Scene source: " << source_name << "\n";

    // Проверка первого посещения
    bool first_visit = (state_.visited_scenes.find(scene_id) == state_.visited_scenes.end());
    bool show_text = (scene_id == "main_menu") || first_visit;

    // Отображение текста сцены
    if (show_text && scene_data.contains("text")) {
        if (scene_data["text"].is_array()) {
            for (const auto& line : scene_data["text"]) {
                std::cout << line.get<std::string>() << "\n";
            }
        }
        else if (scene_data["text"].is_string()) {
            std::cout << scene_data["text"].get<std::string>() << "\n";
        }
        std::cout << "\n";
    }

    // Помечаем сцену как посещенную
    if (first_visit) {
        state_.visited_scenes[scene_id] = true;
    }

    if (scene_data.contains("auto_action")) {
        std::string current_before = state_.current_scene;
        HandleAutoAction(scene_data["auto_action"].get<std::string>());

        // Проверяем изменения после авто-действия
        if (state_.quit_game || state_.current_scene != current_before) {
            return;
        }
    }

    // Обработка выборов
    if (scene_data.contains("choices")) {
        ProcessSceneChoices(scene_data["choices"]);
    }
    // Обработка прямых переходов
    else if (scene_data.contains("next_scene")) {
        state_.current_scene = scene_data["next_scene"].get<std::string>();
    }
    else if (scene_data.contains("next_target")) {
        state_.current_scene = scene_data["next_target"].get<std::string>();
    }
    else if (scene_data.contains("next_check")) {
        ProcessCheck(scene_data["next_check"]);
    }
    // Переход по умолчанию
    else {
        state_.current_scene = "main_menu";
    }

    std::cout << "[DEBUG] Next scene: " << state_.current_scene << "\n";
}

void GameProcessor::ProcessSceneChoices(const nlohmann::json& choices) {
    // Подготовка доступных вариантов
    std::vector<SceneChoice> available_choices;
    int choice_number = 1;

    // 1. Сбор доступных вариантов с проверкой условий
    for (const auto& choice : choices) {
        bool condition_met = true;

        // Проверка условий
        if (choice.contains("condition")) {
            condition_met = EvaluateCondition(choice["condition"].get<std::string>());
        }

        // Добавление в доступные варианты
        if (condition_met) {
            SceneChoice sc;
            sc.number = choice_number++;
            sc.text = choice["text"].get<std::string>();
            sc.data = choice;
            available_choices.push_back(sc);
        }
    }

    // 2. УБРАНО ДОБАВЛЕНИЕ ИНВЕНТАРЯ В ОБЫЧНЫХ СЦЕНАХ
    // Инвентарь теперь доступен ТОЛЬКО в бою

    // 3. Если нет доступных вариантов
    if (available_choices.empty()) {
        std::cerr << "WARNING: No available choices, returning to main menu\n";
        state_.current_scene = "main_menu";
        return;
    }

    // 4. Отображение вариантов
    std::cout << "\nВарианты действий:\n";
    for (const auto& choice : available_choices) {
        std::cout << choice.number << ". " << choice.text << "\n";
    }

    // 5. Получение выбора игрока
    std::cout << "\nВаш выбор: ";
    int selected_index = rpg_utils::Input::GetInt(1, available_choices.size());
    const auto& selected_choice = available_choices[selected_index - 1].data;

    // 6. УБРАНА ОБРАБОТКА ИНВЕНТАРЯ В ОБЫЧНЫХ СЦЕНАХ

    // 7. Применение эффектов выбора
    if (selected_choice.contains("effects")) {
        ApplyGameEffects(selected_choice["effects"]);
    }

    // 8. Обработка проверки характеристики
    if (selected_choice.contains("check")) {
        ProcessCheck(selected_choice["check"]);
    }
    // 9. Обработка прямого перехода
    else if (selected_choice.contains("next_target")) {
        state_.current_scene = selected_choice["next_target"].get<std::string>();
    }
    else if (selected_choice.contains("next_scene")) {
        state_.current_scene = selected_choice["next_scene"].get<std::string>();
    }
    // 10. Обработка специальных действий
    else if (selected_choice.contains("auto_action")) {
        HandleAutoAction(selected_choice["auto_action"].get<std::string>());
    }
    // 11. Переход по умолчанию
    else {
        state_.current_scene = "main_menu";
    }
}

// Новая функция для обработки окончания боя и снятия временных эффектов
void GameProcessor::CleanupCombat(const nlohmann::json& combat_data) {
    // Снимаем все временные бонусы от предметов
    for (const auto& [key, value] : state_.flags) {
        if (key.find("item_bonus_") == 0) {
            std::string item_id = key.substr(11);
            std::string stat_key = "item_bonus_stat_" + item_id;
            std::string value_key = "item_bonus_value_" + item_id;
            std::string original_key = "item_bonus_original_" + item_id;

            if (state_.string_vars.count(stat_key) &&
                state_.string_vars.count(value_key) &&
                state_.string_vars.count(original_key)) {

                std::string stat = state_.string_vars[stat_key];
                int bonus_value = std::stoi(state_.string_vars[value_key]);
                int original_value = std::stoi(state_.string_vars[original_key]);

                // Восстанавливаем исходное значение
                state_.stats[stat] = original_value;
                std::cout << "Эффект предмета закончился. " << stat
                    << " восстановлена до " << original_value << "\n";
            }

            // Удаляем все связанные данные
            state_.flags.erase(key);
            state_.string_vars.erase(stat_key);
            state_.string_vars.erase(value_key);
            state_.string_vars.erase(original_key);
            state_.string_vars.erase("item_bonus_duration_" + item_id);
        }
    }

    // Остальная логика завершения боя
    if (state_.combat.enemy_health <= 0) {
        std::cout << "\nПобеда!\n";
        if (combat_data.contains("on_win")) {
            ApplyCombatResults(combat_data["on_win"]);
        }
    }
    else {
        std::cout << "\nПоражение!\n";
        if (combat_data.contains("on_lose") && combat_data["on_lose"].contains("ending")) {
            std::string ending_id = combat_data["on_lose"]["ending"].get<std::string>();
            ShowEnding(ending_id);
        }
        else {
            state_.current_scene = "main_menu";
        }
    }

    // Сбрасываем боевое состояние
    state_.combat = CombatState();
}

void GameProcessor::ProcessPlayerCombatTurn(const nlohmann::json& combat) {
    const auto& options = combat["player_turn"]["options"];
    std::cout << "\n=== ВАШ ХОД ===\n";

    // 1. Отображение стандартных боевых опций
    std::vector<CombatOption> combat_options;
    int option_index = 1;

    for (const auto& option : options) {
        CombatOption co;
        co.number = option_index++;
        co.text = option["name"].get<std::string>();
        co.data = option;
        co.type = "action";
        combat_options.push_back(co);

        std::cout << co.number << ". " << co.text << "\n";
    }

    // 2. Добавляем опцию использования инвентаря
    CombatOption inventory_option;
    inventory_option.number = option_index++;
    inventory_option.text = "Использовать инвентарь";
    inventory_option.type = "inventory";
    combat_options.push_back(inventory_option);

    std::cout << inventory_option.number << ". " << inventory_option.text << "\n";

    // 3. Получение выбора игрока
    std::cout << "\nВаш выбор (1-" << combat_options.size() << "): ";
    int choice = rpg_utils::Input::GetInt(1, combat_options.size());
    const auto& selected_option = combat_options[choice - 1];

    // 4. Обработка выбора инвентаря
    if (selected_option.type == "inventory") {
        UseCombatInventory();
        return;
    }

    // 5. Обработка стандартных боевых действий
    const auto& action = selected_option.data;
    std::string action_type = action["type"].get<std::string>();
    state_.string_vars["last_player_action"] = action_type;

    bool success = true;
    std::string result_key = "success";
    int difficulty_modifier = 0;

    if (action.contains("check_stat")) {
        const std::string stat = action["check_stat"].get<std::string>();
        const int stat_value = state_.stats.at(stat);

        if (action.contains("difficulty")) {
            difficulty_modifier = action["difficulty"].get<int>();
        }

        if (action_type == "shoot" && state_.flags.count("close_combat") && state_.flags["close_combat"]) {
            difficulty_modifier -= 4;
        }
        else if (action_type == "melee" && state_.flags.count("enemy_fleeing") && state_.flags["enemy_fleeing"]) {
            difficulty_modifier += 2;
        }

        auto result = rpg_utils::RollDiceWithModifiers(stat_value, difficulty_modifier);

        switch (result.result) {
        case rpg_utils::RollResultType::kCriticalSuccess:
            result_key = "critical_success"; break;
        case rpg_utils::RollResultType::kCriticalFail:
            result_key = "critical_fail"; break;
        case rpg_utils::RollResultType::kSuccess:
            result_key = "success"; break;
        case rpg_utils::RollResultType::kFail:
            result_key = "fail"; break;
        }

        std::cout << "\nБросок " << stat << " (" << stat_value;
        if (difficulty_modifier != 0) {
            std::cout << (difficulty_modifier > 0 ? "+" : "") << difficulty_modifier;
        }
        std::cout << "): " << result.total_roll << " -> " << result_key << "\n";

        if (action["results"].contains(result_key)) {
            std::cout << action["results"][result_key].get<std::string>() << "\n";
        }

        success = (result_key == "success" || result_key == "critical_success");
    }

    if (success) {
        if (action.contains("on_success")) {
            ApplyGameEffects(action["on_success"]);
        }
        else if (action.contains("damage")) {
            int damage = rpg_utils::CalculateDamage(action["damage"].get<std::string>());
            state_.combat.enemy_health -= damage;
            std::cout << "Нанесено урона: " << damage << "\n";
        }
    }
    else {
        if (action.contains("on_fail")) {
            ApplyGameEffects(action["on_fail"]);
        }
    }

    state_.combat.player_turn = false;
}

void GameProcessor::UseCombatInventory() {
    const auto& items_data = data_.Get("items");

    // Собираем только consumable-предметы
    std::vector<std::pair<std::string, int>> usable_items;
    for (const auto& [item_id, count] : state_.inventory) {
        if (items_data.contains(item_id)) {
            const auto& item = items_data[item_id];
            if (item.contains("type") && item["type"].get<std::string>() == "consumable") {
                usable_items.push_back({ item_id, count });
            }
        }
    }

    // Если нет расходников
    if (usable_items.empty()) {
        std::cout << "\nУ вас нет расходников!\n";
        state_.combat.player_turn = true; // Игрок может выбрать другое действие
        return;
    }

    // Преобразуем в вектор для упорядоченного отображения
    std::vector<std::pair<std::string, int>> item_list;
    for (const auto& [item_id, count] : usable_items) {
        item_list.push_back({ item_id, count });
    }

    // Отображаем доступные предметы
    std::cout << "\n===== ВАШ ИНВЕНТАРЬ =====\n";
    for (size_t i = 0; i < item_list.size(); i++) {
        const auto& item = items_data[item_list[i].first];
        std::cout << (i + 1) << ". " << item["name"].get<std::string>()
            << " (x" << item_list[i].second << ")\n";

        if (item.contains("description")) {
            std::cout << "   Описание: " << item["description"].get<std::string>() << "\n";
        }

        if (item.contains("combat_effect")) {
            std::cout << "   Эффект: " << item["combat_effect"].get<std::string>() << "\n";
        }
    }
    std::cout << "0. Отмена\n";
    std::cout << "=========================\n";

    // Получаем выбор игрока
    std::cout << "\nВыберите предмет (0 - отмена): ";
    int choice = rpg_utils::Input::GetInt(0, item_list.size());

    if (choice == 0) {
        state_.combat.player_turn = true; // Возвращаем ход игроку
        return;
    }

    // Получаем выбранный предмет
    const std::string& item_id = item_list[choice - 1].first;
    const auto& item_data = items_data[item_id];

    // Применяем эффекты предмета
    ApplyInventoryItemEffects(item_id, item_data);

    // Передаем ход противнику
    state_.combat.player_turn = false;
}

void GameProcessor::ApplyInventoryItemEffects(const std::string& item_id, const nlohmann::json& item_data) {
    bool effect_applied = false;

    // Лечение
    if (item_data.contains("heal")) {
        int heal_amount = rpg_utils::CalculateDamage(item_data["heal"].get<std::string>());
        state_.current_health = std::min(state_.max_health, state_.current_health + heal_amount);
        std::cout << "Вы восстановили " << heal_amount << " здоровья!\n";
        effect_applied = true;
    }

    // Нанесение урона
    if (item_data.contains("damage")) {
        int damage = rpg_utils::CalculateDamage(item_data["damage"].get<std::string>());
        state_.combat.enemy_health -= damage;
        std::cout << "Нанесено урона: " << damage << "!\n";
        effect_applied = true;
    }

    // Временные бонусы
    if (item_data.contains("stat_bonus")) {
        const auto& bonus = item_data["stat_bonus"];
        std::string stat = bonus["stat"].get<std::string>();
        int value = bonus["value"].get<int>();
        int duration = bonus["duration"].get<int>();

        // Сохраняем исходное значение
        int original_value = state_.stats[stat];

        // Применяем бонус
        state_.stats[stat] += value;
        std::cout << "Ваша " << stat << " временно увеличена на " << value << "!\n";

        // Сохраняем информацию для отмены эффекта
        state_.flags["item_bonus_" + item_id] = true;
        state_.string_vars["item_bonus_stat_" + item_id] = stat;
        state_.string_vars["item_bonus_value_" + item_id] = std::to_string(value);
        state_.string_vars["item_bonus_duration_" + item_id] = std::to_string(duration);
        state_.string_vars["item_bonus_original_" + item_id] = std::to_string(original_value);

        effect_applied = true;
    }

    // Специальные эффекты
    if (item_data.contains("combat_effects")) {
        ApplyGameEffects(item_data["combat_effects"]);
        effect_applied = true;
    }

    // Если не применено ни одного эффекта
    if (!effect_applied) {
        std::cout << "Предмет не дал эффекта!\n";
    }

    // Удаление использованного предмета
    if (item_data.contains("consumable") && item_data["consumable"].get<bool>()) {
        // ЗАМЕНА: используем map::find вместо std::find
        auto it = state_.inventory.find(item_id);
        if (it != state_.inventory.end()) {
            // Уменьшаем количество или удаляем предмет
            if (it->second > 1) {
                it->second--;
            }
            else {
                state_.inventory.erase(it);
            }
            std::cout << "Предмет использован (осталось: "
                << (state_.inventory.count(item_id) ? state_.inventory[item_id] : 0) << ")\n";
        }
    }
}

// Добавление предмета(ов)
void GameProcessor::AddItemToInventory(const std::string& item_id, int count) {
    state_.inventory[item_id] += count;
}
// Удаление предмета(ов)
void GameProcessor::RemoveItemFromInventory(const std::string& item_id, int count) {
    if (state_.inventory.find(item_id) != state_.inventory.end()) {
        state_.inventory[item_id] -= count;
        if (state_.inventory[item_id] <= 0) {
            state_.inventory.erase(item_id);
        }
    }
}
// Проверка наличия предмета
bool GameProcessor::HasItem(const std::string& item_id, int count) const {
    auto it = state_.inventory.find(item_id);
    return it != state_.inventory.end() && it->second >= count;
}
// Показать инвентарь
void GameProcessor::ShowInventory() {
    std::cout << "\n===== ВАШ ИНВЕНТАРЬ =====\n";

    if (state_.inventory.empty()) {
        std::cout << "Инвентарь пуст\n";
        std::cout << "=======================\n";
        return;
    }

    const auto& items_data = data_.Get("items");
    for (const auto& [item_id, count] : state_.inventory) {
        if (items_data.contains(item_id)) {
            const auto& item = items_data[item_id];
            std::cout << "- " << item["name"].get<std::string>()
                << " (" << item["type"].get<std::string>() << ", x" << count << ")\n";

            if (item.contains("description")) {
                std::cout << "  Описание: " << item["description"].get<std::string>() << "\n";
            }
        }
    }
    std::cout << "=======================\n";
}