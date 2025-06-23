#include "GameProcessor.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <iomanip>

GameProcessor::GameProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void GameProcessor::ProcessCombat(const std::string& combat_id) {
    const auto& combats = data_.Get("combat");
    if (!combats.contains(combat_id)) {
        std::cerr << "Бой не найден: " << combat_id << std::endl;
        return;
    }

    const auto& combat_data = combats[combat_id];
    InitializeCombat(combat_data, combat_id);

    while (state_.combat.enemy_health > 0 && state_.current_health > 0) {
        DisplayCombatStatus(combat_data);

        if (state_.combat.player_turn) {
            ProcessPlayerTurn(combat_data);
        }
        else {
            ProcessEnemyTurn(combat_data);
        }
    }

    CleanupCombat(combat_data);
}

void GameProcessor::ShowEnding(const std::string& ending_id) {
    const auto& endings = data_.Get("endings");
    if (!endings.contains(ending_id)) {
        std::cerr << "Концовка не найдена: " << ending_id << std::endl;
        return;
    }

    const auto& ending = endings[ending_id];
    DisplayEnding(ending);
    state_.unlocked_endings.insert(ending_id);
    state_.quit_game = true;
}

void GameProcessor::ShowEndingCollection() {
    const auto& endings = data_.Get("endings");
    std::cout << "\n===== ДОСТИГНУТЫЕ КОНЦОВКИ =====\n";

    for (const auto& id : state_.unlocked_endings) {
        if (endings.contains(id)) {
            const auto& ending = endings[id];
            std::cout << ending["title"].get<std::string>() << "\n";
            std::cout << "------------------------------\n";
        }
    }

    std::cout << "\nНажмите Enter для продолжения...";
    rpg_utils::Input::GetLine();
}

void GameProcessor::InitializeCharacter() {
    const auto& char_base = data_.Get("character_base");
    state_.stats = char_base["base_stats"].get<std::unordered_map<std::string, int>>();
    state_.stat_points = char_base["points_to_distribute"].get<int>();

    const auto& core_stats = char_base["core_stats"].get<std::vector<std::string>>();
    const auto& display_names = char_base["display_names"].get<std::unordered_map<std::string, std::string>>();
    const auto& descriptions = char_base["descriptions"].get<std::unordered_map<std::string, std::string>>();
    const auto& name_lengths = char_base["name_lengths"].get<std::unordered_map<std::string, int>>();

    // Выводим описания характеристик
    std::cout << "\n===== ОПИСАНИЕ ХАРАКТЕРИСТИК =====\n";
    for (const auto& stat : core_stats) {
        std::cout << display_names.at(stat) << ":\n";
        std::cout << descriptions.at(stat) << "\n\n";
    }

    // Определяем максимальную ширину колонок
    int max_name_width = 0;
    for (const auto& stat : core_stats) {
        max_name_width = std::max(max_name_width, name_lengths.at(stat));
    }
    max_name_width += 4;  // Добавляем отступ

    while (state_.stat_points > 0) {
        std::cout << "\n===== СОЗДАНИЕ ПЕРСОНАЖА =====\n";
        std::cout << "Осталось очков: " << state_.stat_points << "\n\n";

        // Шапка таблицы
        std::cout << std::left << std::setw(4) << "ID"
            << std::setw(max_name_width) << "Характеристика"
            << std::setw(8) << "Текущее"
            << "Описание\n";

        std::cout << std::setfill('-') << std::setw(4 + max_name_width + 8 + 50) << ""
            << std::setfill(' ') << "\n";

        // Данные характеристик
        int index = 1;
        for (const auto& stat : core_stats) {
            std::cout << std::left << std::setw(4) << index
                << std::setw(max_name_width) << display_names.at(stat)
                << std::setw(8) << state_.stats[stat]
                << descriptions.at(stat) << "\n";
                index++;
        }

        // Выбор характеристик
        std::cout << "\nВыберите характеристику (1-" << core_stats.size() << "): ";
        int stat_index = rpg_utils::Input::GetInt(1, core_stats.size()) - 1;

        std::cout << "Сколько очков добавить (1-" << state_.stat_points << "): ";
        int points = rpg_utils::Input::GetInt(1, state_.stat_points);

        std::string chosen_stat = core_stats[stat_index];
        state_.stats[chosen_stat] += points;
        state_.stat_points -= points;
    }

    CalculateDerivedStats();
    std::cout << "\nПерсонаж успешно создан!\n\n";
}

void GameProcessor::DisplayScene(const nlohmann::json& scene) {
    std::cout << "\n";
    for (const auto& line : scene["text"]) {
        std::cout << line.get<std::string>() << "\n";
    }
    std::cout << "\n";
}

void GameProcessor::HandleAutoAction(const std::string& action) {
    if (action == "start_creation") {
        InitializeCharacter();
    }
    else if (action == "show_endings") {
        ShowEndingCollection();
    }
}

void GameProcessor::ApplyGameEffects(const nlohmann::json& effects) {
    // Устанавливаем флаги
    if (effects.contains("set_flags")) {
        for (const auto& [flag, value] : effects["set_flags"].items()) {
            state_.flags[flag] = value.get<bool>();
            std::cout << "Установлен флаг: " << flag
                << " = " << (value.get<bool>() ? "true" : "false") << "\n";
        }
    }

    // Добавляем предметы в инвентарь
    if (effects.contains("add_items")) {
        for (const auto& item : effects["add_items"]) {
            std::string item_id = item.get<std::string>();
            state_.inventory.push_back(item_id);

            // Получаем информацию о предмете
            const auto& items = data_.Get("items");
            if (items.contains(item_id)) {
                std::cout << "Получен предмет: "
                    << items[item_id]["name"].get<std::string>() << "\n";
            }
        }
    }
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
    state_.current_health = state_.derived_stats.at("health");
}

void GameProcessor::InitializeCombat(const nlohmann::json& combat_data, const std::string& combat_id) {
    state_.combat.enemy_id = combat_id;
    state_.combat.enemy_health = combat_data["health"].get<int>();
    state_.combat.current_phase = 0;
    state_.combat.player_turn = true;
}

void GameProcessor::DisplayCombatStatus(const nlohmann::json& combat_data) {
    std::cout << "\n===== БОЙ =====\n";
    std::cout << "Противник: " << combat_data["enemy"].get<std::string>() << "\n";
    std::cout << "Здоровье противника: " << state_.combat.enemy_health << "/"
        << combat_data["health"].get<int>() << "\n";
    std::cout << "Ваше здоровье: " << state_.current_health << "/"
        << state_.derived_stats.at("health") << "\n\n";
}

void GameProcessor::CleanupCombat(const nlohmann::json& combat_data) {
    if (state_.combat.enemy_health <= 0) {
        std::cout << "\nПобеда!\n";
        if (combat_data.contains("on_win")) {
            ApplyGameEffects(combat_data["on_win"]);
        }
    }
    else {
        std::cout << "\nПоражение!\n";
        state_.current_scene = combat_data["on_lose"].get<std::string>();
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

std::vector<GameProcessor::SceneChoice> GameProcessor::GetAvailableChoices(const nlohmann::json& choices) {
    std::vector<SceneChoice> available;

    for (const auto& choice : choices) {
        // Проверяем условие, если оно есть
        bool condition_met = true;
        if (choice.contains("condition")) {
            condition_met = EvaluateCondition(choice["condition"].get<std::string>());
        }

        if (condition_met) {
            SceneChoice sc;
            sc.id = choice.value("id", "");
            sc.text = choice["text"].get<std::string>();
            sc.effects = choice.value("effects", nlohmann::json());
            sc.check = choice.value("check", nlohmann::json());

            // Определяем цель перехода
            if (choice.contains("next_target")) {
                sc.next_target = choice["next_target"].get<std::string>();
            }
            else if (!sc.check.empty() && sc.check.contains("results")) {
                // Для проверок используем первый доступный результат
                const auto& results = sc.check["results"];
                if (results.is_object() && !results.empty()) {
                    sc.next_target = results.begin()->get<std::string>();
                }
            }

            available.push_back(sc);
        }
    }

    return available;
}

void GameProcessor::ProcessPlayerChoice(const std::vector<SceneChoice>& choices) {
    // Выводим доступные варианты выбора
    std::cout << "\nВарианты действий:\n";
    for (size_t i = 0; i < choices.size(); i++) {
        std::cout << i + 1 << ". " << choices[i].text << "\n";
    }

    // Получаем выбор игрока
    std::cout << "\nВаш выбор: ";
    int choice_index = rpg_utils::Input::GetInt(1, static_cast<int>(choices.size())) - 1;
    const auto& choice = choices[choice_index];

    // Применяем эффекты выбора
    if (!choice.effects.is_null()) {
        ApplyGameEffects(choice.effects);
    }

    // Обрабатываем проверку характеристики, если она есть
    if (!choice.check.is_null()) {
        ProcessCheck(choice.check);
    }
    // Если проверки нет, переходим к следующей сцене
    else if (!choice.next_target.empty()) {
        state_.current_scene = choice.next_target;
    }
    else {
        std::cerr << "Ошибка: не определена следующая сцена для выбора\n";
        state_.current_scene = "main_menu";
    }
}

bool GameProcessor::EvaluateCondition(const std::string& condition) {
    // Сложные условия с логическими операторами
    if (condition.find("&&") != std::string::npos) {
        std::vector<std::string> sub_conditions = rpg_utils::Split(condition, '&');
        for (const auto& sub_cond : sub_conditions) {
            if (!EvaluateSingleCondition(sub_cond)) {
                return false;
            }
        }
        return true;
    }
    else if (condition.find("||") != std::string::npos) {
        std::vector<std::string> sub_conditions = rpg_utils::Split(condition, '|');
        for (const auto& sub_cond : sub_conditions) {
            if (EvaluateSingleCondition(sub_cond)) {
                return true;
            }
        }
        return false;
    }

    return EvaluateSingleCondition(condition);
}

bool GameProcessor::EvaluateSingleCondition(const std::string& condition) {
    std::string cond = rpg_utils::Trim(condition);

    // Обработка отрицания
    bool negate = false;
    if (cond[0] == '!') {
        negate = true;
        cond = cond.substr(1);
    }

    bool result = false;

    // Условия с флагами
    if (cond.find("flags.") == 0) {
        std::string flag = cond.substr(6);
        result = state_.flags.count(flag) ? state_.flags.at(flag) : false;
    }
    // Проверка наличия предмета в инвентаре
    else if (cond.find("inventory.") == 0) {
        std::string item = cond.substr(10);
        result = std::find(state_.inventory.begin(),
            state_.inventory.end(),
            item) != state_.inventory.end();
    }

    return negate ? !result : result;
}

void GameProcessor::ResetVisitedScenes() {
    visited_scenes_.clear();
}

void GameProcessor::ProcessCheckResult(const nlohmann::json& result) {
    // Применяем эффекты
    if (result.contains("set_flags")) {
        ApplyGameEffects({ {"set_flags", result["set_flags"]} });
    }
    if (result.contains("add_items")) {
        ApplyGameEffects({ {"add_items", result["add_items"]} });
    }

    // Определяем следующее действие
    if (result.contains("choices")) {
        std::cout << "Варианты действий:\n";
        auto choices = GetAvailableChoices(result["choices"]);
        if (!choices.empty()) {
            ProcessPlayerChoice(choices);
        }
        else {
            std::cout << "Нет доступных действий. Возврат в главное меню.\n";
            state_.current_scene = "main_menu";
        }
    }
    else if (result.contains("next_scene")) {
        state_.current_scene = result["next_scene"].get<std::string>();
    }
    else if (result.contains("next_check")) {
        ProcessCheck(result["next_check"]);
    }
    else {
        std::cerr << "Ошибка: не определено следующее действие для результата\n";
        state_.current_scene = "main_menu";
    }
}

void GameProcessor::ProcessScene(const std::string& scene_id) {
    // Проверяем, посещали ли мы эту сцену ранее
    bool is_first_visit = (visited_scenes_.find(scene_id) == visited_scenes_.end());

    // Пробуем найти в обычных сценах
    const auto& scenes = data_.Get("scenes");
    if (scenes.contains(scene_id)) {
        const auto& scene = scenes[scene_id];

        // Выводим описание только при первом посещении
        if (is_first_visit) {
            DisplayScene(scene);
            visited_scenes_[scene_id] = true;
        }

        if (scene.contains("auto_action")) {
            HandleAutoAction(scene["auto_action"].get<std::string>());
        }

        if (scene.contains("choices")) {
            auto choices = GetAvailableChoices(scene["choices"]);
            if (!choices.empty()) {
                ProcessPlayerChoice(choices);
                return;
            }
            else {
                std::cout << "Нет доступных действий. Возврат в главное меню.\n";
                state_.current_scene = "main_menu";
            }
        }
        // Если в сцене нет choices, но есть next_scene
        else if (scene.contains("next_scene")) {
            state_.current_scene = scene["next_scene"].get<std::string>();
            return;
        }
        else {
            std::cout << "Сцена не содержит вариантов выбора. Возврат в главное меню.\n";
            state_.current_scene = "main_menu";
        }
        return;
    }

    // Пробуем найти в проверках (checks)
    const auto& checks = data_.Get("checks");
    if (checks.contains(scene_id)) {
        const auto& result = checks[scene_id];

        // Для результатов проверок всегда выводим текст при первом посещении
        if (is_first_visit) {
            if (result.contains("text")) {
                std::cout << "\n" << result["text"].get<std::string>() << "\n\n";
            }
            visited_scenes_[scene_id] = true;
        }

        ProcessCheckResult(result);
        return;
    }

    // Если не нашли нигде
    std::cerr << "Сцена не найдена: " << scene_id << std::endl;
    std::cout << "Возврат в главное меню.\n";
    state_.current_scene = "main_menu";
}

void GameProcessor::ProcessCheck(const nlohmann::json& check_data) {
    // Проверяем, есть ли поле "type" (новый формат)
    if (check_data.contains("type")) {
        std::string stat = check_data["type"].get<std::string>();
        int difficulty = check_data.value("difficulty", 0);
        int base_value = state_.stats.at(stat);

        // Бросаем кубики в стиле GURPS
        auto roll = rpg_utils::RollDiceWithModifiers(base_value, difficulty);
        std::cout << "\nБросок характеристики " << stat << ": "
            << roll.total_roll << " против " << base_value << "\n";

        // Определяем результат броска
        std::string result_type;
        switch (roll.result) {
        case rpg_utils::RollResultType::kCriticalSuccess:
            result_type = "critical_success";
            std::cout << "Критический успех!\n";
            break;
        case rpg_utils::RollResultType::kCriticalFail:
            result_type = "critical_fail";
            std::cout << "Критическая неудача!\n";
            break;
        case rpg_utils::RollResultType::kSuccess:
            result_type = "success";
            std::cout << "Успех!\n";
            break;
        case rpg_utils::RollResultType::kFail:
            result_type = "fail";
            std::cout << "Неудача!\n";
            break;
        }

        // Определяем следующую сцену
        if (check_data.contains("results") && check_data["results"].is_object()) {
            // Новый формат с объектом results
            if (check_data["results"].contains(result_type)) {
                state_.current_scene = check_data["results"][result_type].get<std::string>();
            }
            else {
                std::cerr << "Ошибка: не определена следующая сцена для результата "
                    << result_type << std::endl;
                state_.current_scene = "main_menu";
            }
        }
        else {
            // Старый формат - результаты в корне объекта
            if (check_data.contains(result_type)) {
                state_.current_scene = check_data[result_type].get<std::string>();
            }
            else if (result_type == "success" && check_data.contains("success")) {
                state_.current_scene = check_data["success"].get<std::string>();
            }
            else if (result_type == "fail" && check_data.contains("fail")) {
                state_.current_scene = check_data["fail"].get<std::string>();
            }
            else {
                std::cerr << "Ошибка: не определена следующая сцена для результата "
                    << result_type << std::endl;
                state_.current_scene = "main_menu";
            }
        }
    }
    // Обработка старого формата (прямые ссылки)
    else {
        // Упрощенная обработка для старого формата
        std::string result_type = "success"; // По умолчанию считаем успех

        // Если указан тип проверки, делаем реальный бросок
        if (check_data.contains("type")) {
            std::string stat = check_data["type"].get<std::string>();
            int difficulty = check_data.value("difficulty", 0);
            int base_value = state_.stats.at(stat);

            auto roll = rpg_utils::RollDiceWithModifiers(base_value, difficulty);
            std::cout << "\nБросок характеристики " << stat << ": "
                << roll.total_roll << " против " << base_value << "\n";

            switch (roll.result) {
            case rpg_utils::RollResultType::kCriticalSuccess:
                result_type = "critical_success";
                std::cout << "Критический успех!\n";
                break;
            case rpg_utils::RollResultType::kCriticalFail:
                result_type = "critical_fail";
                std::cout << "Критическая неудача!\n";
                break;
            case rpg_utils::RollResultType::kSuccess:
                result_type = "success";
                std::cout << "Успех!\n";
                break;
            case rpg_utils::RollResultType::kFail:
                result_type = "fail";
                std::cout << "Неудача!\n";
                break;
            }
        }

        // Переходим к соответствующей сцене
        if (check_data.contains(result_type)) {
            state_.current_scene = check_data[result_type].get<std::string>();
        }
        else if (result_type == "success" && check_data.contains("success")) {
            state_.current_scene = check_data["success"].get<std::string>();
        }
        else if (result_type == "fail" && check_data.contains("fail")) {
            state_.current_scene = check_data["fail"].get<std::string>();
        }
        else {
            std::cerr << "Ошибка: не определена следующая сцена для результата "
                << result_type << std::endl;
            state_.current_scene = "main_menu";
        }
    }
}

void GameProcessor::ProcessPlayerTurn(const nlohmann::json& combat_data) {
    const auto& options = combat_data["player_turn"]["options"];
    std::cout << "Ваш ход:\n";

    for (size_t i = 0; i < options.size(); i++) {
        std::cout << i + 1 << ". " << options[i]["name"].get<std::string>() << "\n";
    }

    std::cout << "\nВыберите действие: ";
    int choice = rpg_utils::Input::GetInt(1, options.size()) - 1;
    const auto& action = options[choice];

    int base_value = state_.stats.at(action["stat"].get<std::string>());
    int difficulty = action.value("difficulty", 0);

    // Бросок кубиков в стиле GURPS
    auto roll = rpg_utils::RollDiceWithModifiers(base_value, difficulty);

    // Получаем результат броска
    std::string result_key;
    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess: result_key = "critical_success"; break;
    case rpg_utils::RollResultType::kCriticalFail: result_key = "critical_fail"; break;
    case rpg_utils::RollResultType::kSuccess: result_key = "success"; break;
    case rpg_utils::RollResultType::kFail: result_key = "fail"; break;
    }

    if (action["results"].contains(result_key)) {
        std::cout << "\n" << action["results"][result_key].get<std::string>() << "\n";
    }

    if (action.contains("damage")) {
        // Используем упрощенную систему урона GURPS
        int damage = rpg_utils::CalculateDamage(action["damage"].get<std::string>());
        state_.combat.enemy_health -= damage;
        std::cout << "Нанесено урона: " << damage << "\n";
    }

    state_.combat.player_turn = false;
}

void GameProcessor::ProcessEnemyTurn(const nlohmann::json& combat_data) {
    const auto& phases = combat_data["phases"];
    int current_phase = 0;

    // Находим текущую фазу боя на основе здоровья
    for (size_t i = 0; i < phases.size(); i++) {
        if (state_.combat.enemy_health <= phases[i]["health_threshold"]) {
            current_phase = static_cast<int>(i);
            break;
        }
    }

    const auto& attacks = phases[current_phase]["attacks"];
    if (attacks.empty()) return;

    // Выбираем случайную атаку для этой фазы
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, attacks.size() - 1);
    const auto& attack = attacks[dist(gen)];

    std::cout << "\nПротивник использует: " << attack["name"].get<std::string>() << "!\n";
    std::cout << attack["description"].get<std::string>() << "\n";

    if (attack.contains("damage")) {
        // Упрощенный расчет урона в стиле GURPS
        int damage = rpg_utils::CalculateDamage(attack["damage"].get<std::string>());
        state_.current_health -= damage;
        std::cout << "Получено урона: " << damage << "\n";
    }

    state_.combat.player_turn = true;
}
