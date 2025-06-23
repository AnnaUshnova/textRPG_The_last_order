#include "CombatProcessor.h"
#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>
#include <random>
#include <sstream>

using namespace rpg_utils;
using json = nlohmann::json;

CombatProcessor::CombatProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void CombatProcessor::Process(const std::string& combat_id) {
    InitializeCombat(combat_id);
    RunCombatLoop();
}

void CombatProcessor::InitializeCombat(const std::string& combat_id) {
    // Загрузка данных боя
    combat_data_ = data_.Get("combat", combat_id);

    // Инициализация состояния боя
    state_.combat.enemy_id = combat_id;
    state_.combat.enemy_health = combat_data_["health"];
    state_.combat.current_phase = 0;
    state_.combat.player_turn = true;

    // Загрузка фаз боя
    enemy_phases_.clear();
    for (const auto& phase : combat_data_["phases"]) {
        CombatPhase cp;
        cp.health_threshold = phase["health_threshold"];
        cp.attacks = phase["attacks"];
        enemy_phases_.push_back(cp);
    }

    // Загрузка эффектов окружения
    environment_effects_.clear();
    for (const auto& env : combat_data_["environment"]) {
        EnvironmentEffect effect;
        effect.name = env["name"];
        effect.description = env.value("description", "");
        effect.effect = env["effect"];
        environment_effects_.push_back(effect);
    }

    // Начальное сообщение
    std::cout << "\nБОЙ С " << combat_data_["enemy"].get<std::string>() << "!\n";
    std::cout << "=======================================\n";
}

void CombatProcessor::RunCombatLoop() {
    while (state_.combat.enemy_health > 0 && state_.derived_stats["health"] > 0) {
        DisplayCombatStatus();

        if (state_.combat.player_turn) {
            ProcessPlayerTurn();
        }
        else {
            ProcessEnemyTurn();
        }

        // Проверка смены фазы
        int new_phase = GetCurrentEnemyPhase();
        if (new_phase != state_.combat.current_phase) {
            state_.combat.current_phase = new_phase;
            std::cout << "\nФаза боя изменилась! "
                << combat_data_["phases"][new_phase]["phase_name"] << "\n";
        }
    }

    // Определение результата боя
    bool player_won = (state_.combat.enemy_health <= 0);
    CleanupCombat(player_won);
}

void CombatProcessor::DisplayCombatStatus() const {
    std::cout << "\n---------------------------------------\n";
    std::cout << "Ваше здоровье: " << state_.derived_stats["health"] << "/"
        << (state_.stats["endurance"] * 2) << "\n";
    std::cout << "Здоровье противника: " << state_.combat.enemy_health << "/"
        << combat_data_["health"] << "\n";
    std::cout << "Фаза боя: " << (state_.combat.current_phase + 1)
        << "/" << enemy_phases_.size() << "\n";
    std::cout << "---------------------------------------\n";
}

void CombatProcessor::ProcessPlayerTurn() {
    std::cout << "\n=== ВАШ ХОД ===\n";
    DisplayPlayerOptions();

    const auto& options = combat_data_["player_turn"]["options"];
    int choice = Input::GetInt(1, options.size()) - 1;

    ExecutePlayerAction(options[choice]);
    state_.combat.player_turn = false;
}

void CombatProcessor::DisplayPlayerOptions() const {
    const auto& options = combat_data_["player_turn"]["options"];

    std::cout << "Варианты действий:\n";
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << i + 1 << ". " << options[i]["name"].get<std::string>() << "\n";
    }

    // Отображение эффектов окружения
    if (!environment_effects_.empty()) {
        std::cout << "\nОсобенности окружения:\n";
        for (size_t i = 0; i < environment_effects_.size(); ++i) {
            std::cout << " - " << environment_effects_[i].name << ": "
                << environment_effects_[i].description << "\n";
        }
    }
}

void CombatProcessor::ExecutePlayerAction(const json& action) {
    std::cout << "\n>> " << action["name"].get<std::string>() << "\n";

    // Проверка успешности действия
    if (action.contains("stat")) {
        int base_value = state_.stats[action["stat"].get<std::string>()];
        int difficulty = action.value("difficulty", 0);

        auto roll = RollDiceWithModifiers(base_value, difficulty);
        std::cout << "Результат броска: " << roll.total_roll
            << " (" << roll.result_str << ")\n";

        // Определение результата на основе броска
        std::string result_key = "results.";
        switch (roll.result) {
        case RollResultType::kCriticalSuccess:
            result_key += "critical_success"; break;
        case RollResultType::kSuccess:
            result_key += "success"; break;
        case RollResultType::kFail:
            result_key += "fail"; break;
        case RollResultType::kCriticalFail:
            result_key += "critical_fail"; break;
        }

        // Применение результатов
        if (action.contains(result_key)) {
            std::cout << action[result_key].get<std::string>() << "\n";
        }

        // Нанесение урона при успехе
        if ((roll.result == RollResultType::kSuccess ||
            roll.result == RollResultType::kCriticalSuccess) &&
            action.contains("damage")) {
            int damage = CalculateDamage(action["damage"].get<std::string>());
            std::cout << "Нанесено урона: " << damage << "\n";
            state_.combat.enemy_health -= damage;
        }

        // Применение эффектов
        if (action.contains("effects")) {
            for (const auto& effect : action["effects"]) {
                ApplyGameEffects(effect.get<std::string>(), state_);
            }
        }
    }
}

void CombatProcessor::ProcessEnemyTurn() {
    std::cout << "\n=== ХОД ПРОТИВНИКА ===\n";

    // Выбор атаки для текущей фазы
    const auto& attacks = enemy_phases_[state_.combat.current_phase].attacks;
    if (attacks.empty()) return;

    // Случайный выбор атаки
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, attacks.size() - 1);
    const auto& attack = attacks[dist(gen)];

    ExecuteEnemyAttack(attack);
    state_.combat.player_turn = true;
}

void CombatProcessor::ExecuteEnemyAttack(const json& attack) {
    std::cout << ">> " << attack["name"].get<std::string>() << "\n";
    std::cout << attack["description"].get<std::string>() << "\n";

    // Расчет атаки
    auto roll = ResolveAttack(
        attack,
        combat_data_["stats"].get<std::unordered_map<std::string, int>>()
    );

    // Отображение результата
    std::cout << "Результат атаки: " << roll.total_roll
        << " (" << roll.result_str << ")\n";

    // Применение урона
    if ((roll.result == RollResultType::kSuccess ||
        roll.result == RollResultType::kCriticalSuccess) &&
        attack.contains("damage")) {
        int damage = CalculateDamage(attack["damage"].get<std::string>());

        // Учет сопротивлений
        std::string damage_type = attack.value("type", "physical");
        damage = std::max(1, damage);

        std::cout << "Получено урона: " << damage << "\n";
        ProcessDamage(damage, true);
    }

    // Применение эффектов
    if (attack.contains("effects")) {
        for (const auto& effect : attack["effects"]) {
            ApplyGameEffects(effect.get<std::string>(), state_);
        }
    }
}





int CombatProcessor::GetCurrentEnemyPhase() const {
    for (int i = enemy_phases_.size() - 1; i >= 0; --i) {
        if (state_.combat.enemy_health <= enemy_phases_[i].health_threshold) {
            return i;
        }
    }
    return 0;
}

void CombatProcessor::ProcessDamage(int damage, bool target_is_player) {
    if (target_is_player) {
        state_.derived_stats["health"] -= damage;
    }
    else {
        state_.combat.enemy_health -= damage;
    }
}

void CombatProcessor::CleanupCombat(bool player_won) {
    // Применение результатов боя
    const std::string& result_key = player_won ? "on_win" : "on_lose";
    const auto& result = combat_data_[result_key];

    // Установка флагов
    if (result.contains("set_flags")) {
        for (const auto& [flag, value] : result["set_flags"].items()) {
            state_.flags[flag] = value.get<bool>();
        }
    }

    // Добавление предметов
    if (result.contains("add_items")) {
        for (const auto& item : result["add_items"]) {
            state_.inventory.push_back(item.get<std::string>());
        }
    }

    // Переход на следующую сцену
    state_.current_scene = result["next_scene"].get<std::string>();

    // Очистка боевых данных
    state_.combat = GameState::CombatState();
}