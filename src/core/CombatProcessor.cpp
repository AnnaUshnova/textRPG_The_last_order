#include "CombatProcessor.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <regex>

CombatProcessor::CombatProcessor(DataManager& data, GameState& state)
    : data_manager_(data), game_state_(state) {
}

void CombatProcessor::Process(const std::string& combat_id) {
    current_combat_ = data_manager_.Get("combat", combat_id);
    InitializeCombat(current_combat_);

    while (game_state_.current_health > 0 && game_state_.combat_enemy_health > 0) {
        PlayerTurn();
        if (game_state_.combat_enemy_health <= 0) break;

        EnemyTurn();
    }

    ProcessCombatResult(game_state_.current_health > 0);
}

void CombatProcessor::InitializeCombat(const nlohmann::json& combat_data) {
    game_state_.combat_enemy = combat_data["enemy"].get<std::string>();
    game_state_.combat_enemy_health = combat_data["health"].get<int>();

    // Инициализация фаз врага
    enemy_phases_.clear();
    for (auto& phase : combat_data["phases"]) {
        CombatPhase p;
        p.health_threshold = phase["health_threshold"].get<int>();
        for (auto& attack : phase["attacks"]) {
            p.attacks.push_back(attack);
        }
        enemy_phases_.push_back(p);
    }

    // Сортировка фаз по убыванию здоровья
    std::sort(enemy_phases_.begin(), enemy_phases_.end(),
        [](const CombatPhase& a, const CombatPhase& b) {
            return a.health_threshold > b.health_threshold;
        });
}

void CombatProcessor::PlayerTurn() {
    // Отображаем текущее здоровье игрока
    std::cout << "Ваше HP: " << game_state_.current_health
        << " | HP врага: " << game_state_.combat_enemy_health << "\n\n";

   auto& options = current_combat_["player_turn"]["options"];


    // Отображение вариантов действий
    int index = 1;
    for (auto& option : options) {
        std::cout << index++ << ". " << option["name"].get<std::string>() << std::endl;
    }

    // Обработка выбора игрока
    int choice = rpg_utils::Input::GetInt(1, options.size()) - 1;
    ExecutePlayerAction(options[choice]);
}

void CombatProcessor::ExecutePlayerAction(const nlohmann::json& action) {
    std::cout << "\n>> " << action["name"].get<std::string>() << "..." << std::endl;

    // Проверка характеристики
    std::string stat = action["stat"].get<std::string>();
    int base_value = game_state_.stats[stat];
    int difficulty = action["difficulty"].get<int>();
    int target_value = base_value + difficulty;

    auto roll = rpg_utils::RollWithDetails(target_value);
    rpg_utils::PrintRollDetails(action["name"].get<std::string>(),
        base_value, difficulty, target_value, roll);

    // Обработка результатов
    std::string result_key;
    switch (roll.result) {
    case rpg_utils::RollResultType::kCriticalSuccess:
        result_key = "critical_success"; break;
    case rpg_utils::RollResultType::kSuccess:
        result_key = "success"; break;
    case rpg_utils::RollResultType::kFail:
        result_key = "fail"; break;
    case rpg_utils::RollResultType::kCriticalFail:
        result_key = "critical_fail"; break;
    }

    if (action["results"].contains(result_key)) {
        std::cout << action["results"][result_key].get<std::string>() << std::endl;
    }

    // Применение урона при успехе
    if ((roll.result == rpg_utils::RollResultType::kCriticalSuccess ||
        roll.result == rpg_utils::RollResultType::kSuccess) &&
        action.contains("damage"))
    {
        int damage = CalculateDamage(action["damage"].get<std::string>());
        game_state_.combat_enemy_health -= damage;
        std::cout << "Нанесено урона: " << damage << " HP" << std::endl;

        if (game_state_.combat_enemy_health < 0) {
            game_state_.combat_enemy_health = 0;
        }
    }
}

void CombatProcessor::EnemyTurn() {
    std::cout << "\n=== ХОД ВРАГА (" << game_state_.combat_enemy << ") ===" << std::endl;

    // Выбор текущей фазы
    int phase_index = GetCurrentEnemyPhase();
    auto& attacks = enemy_phases_[phase_index].attacks;

    // Случайный выбор атаки
    if (!attacks.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, attacks.size() - 1);
        ExecuteEnemyAttack(attacks[distr(gen)]);
    }
}

void CombatProcessor::ExecuteEnemyAttack(const nlohmann::json& attack) {
    std::cout << attack["description"].get<std::string>() << std::endl;
    ResolveAttack(attack, false);
}


void CombatProcessor::ResolveAttack(const nlohmann::json& attack, bool is_player_attacking) {
    // Определение типа урона (по умолчанию физический)
    std::string damage_type = "physical";

    if (attack.contains("type")) {
        std::string attack_type = attack["type"].get<std::string>();

        // Маппинг типов атак на типы урона
        if (attack_type == "energy") damage_type = "energy";
        else if (attack_type == "mental") damage_type = "mental";
    }

    // Проверка характеристики
    std::string stat = attack["stat"].get<std::string>();
    int base_value = current_combat_["stats"][stat].get<int>();
    int difficulty = attack["difficulty"].get<int>();
    int target_value = base_value + difficulty;

    auto roll = rpg_utils::RollWithDetails(target_value);

    // Применение урона при успехе
    if ((roll.result == rpg_utils::RollResultType::kCriticalSuccess ||
        roll.result == rpg_utils::RollResultType::kSuccess) &&
        attack.contains("damage"))
    {
        int raw_damage = CalculateDamage(attack["damage"].get<std::string>());
        int final_damage = raw_damage;

        // Применение сопротивления
        if (is_player_attacking) {
            // Атака игрока по врагу
            if (current_combat_.contains("resistance") &&
                current_combat_["resistance"].contains(damage_type)) {
                int resistance = current_combat_["resistance"][damage_type].get<int>();
                final_damage = std::max(0, raw_damage - resistance);
            }
            game_state_.combat_enemy_health -= final_damage;
            std::cout << "Нанесено урона врагу: " << final_damage << " HP" << std::endl;
        }
        else {
            // Атака врага по игроку
            if (game_state_.resistances.find(damage_type) != game_state_.resistances.end()) {
                int resistance = game_state_.resistances[damage_type];
                final_damage = std::max(0, raw_damage - resistance);
            }
            game_state_.current_health -= final_damage; // Прямое изменение здоровья
            std::cout << "Получено урона (" << damage_type << "): "
                << final_damage << " HP" << std::endl;
        }

        // Корректировка здоровья
        if (is_player_attacking) {
            game_state_.combat_enemy_health = std::max(0, game_state_.combat_enemy_health);
        }
        else {
            game_state_.current_health = std::max(0, game_state_.current_health);
        }
    }
}


int CombatProcessor::GetCurrentEnemyPhase() {
    for (int i = 0; i < enemy_phases_.size(); ++i) {
        if (game_state_.combat_enemy_health <= enemy_phases_[i].health_threshold) {
            return i;
        }
    }
    return enemy_phases_.size() - 1;
}

void CombatProcessor::ProcessCombatResult(bool player_won) {
    if (player_won) {
        std::cout << "\nПОБЕДА! " << game_state_.combat_enemy << " повержен!\n";
        if (current_combat_["on_win"].contains("set_flags")) {
            for (auto& [flag, value] : current_combat_["on_win"]["set_flags"].items()) {
                game_state_.flags[flag] = value.get<bool>();
            }
        }
        // Сохраняем следующую сцену в локальной переменной
        next_scene_ = current_combat_["on_win"]["next_scene"].get<std::string>();
    }
    else {
        std::cout << "\nПОРАЖЕНИЕ...\n";
        next_scene_ = current_combat_["on_lose"].get<std::string>();
    }

    // Обновляем состояние игры для перехода
    game_state_.active_type = "scene";
    game_state_.active_id = next_scene_;
}

int CombatProcessor::CalculateDamage(const std::string& dice_formula) {
    std::regex pattern(R"((\d+)d(\d+)([+-]\d+)?)");
    std::smatch matches;

    if (std::regex_match(dice_formula, matches, pattern)) {
        int count = std::stoi(matches[1]);
        int sides = std::stoi(matches[2]);
        int modifier = 0;

        if (matches[3].matched) {
            modifier = std::stoi(matches[3].str());
        }

        int total = 0;
        for (int i = 0; i < count; ++i) {
            total += (rand() % sides) + 1;
        }

        return total + modifier;
    }

    // Попытка обработать простые числа
    try {
        return std::stoi(dice_formula);
    }
    catch (...) {
        return 0; // Возвращаем 0 при неудаче
    }
}