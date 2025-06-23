#include "Utils.h"
#include "DataManager.h"
#include "GameState.h"
#include "SceneProcessor.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>
#include <sstream>
#include <cmath>
#include <stack>
#include <map>
#include <functional>

namespace rpg_utils {

    using json = nlohmann::json;

    // ===== String Utilities =====
    std::vector<std::string> Split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    std::string ToLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return lower;
    }

    // ===== Dice and Roll Systems =====
    RollDetails RollDiceWithModifiers(
        int base_value,
        int modifier,
        int critical_threshold
    ) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 100);

        RollDetails result;
        result.total_roll = dist(gen);
        int critical_range = critical_threshold;

        if (result.total_roll <= critical_range) {
            result.result = RollResultType::kCriticalSuccess;
            result.result_str = "Критический успех!";
        }
        else if (result.total_roll >= 100 - critical_range) {
            result.result = RollResultType::kCriticalFail;
            result.result_str = "Критическая неудача!";
        }
        else if (result.total_roll + modifier >= base_value) {
            result.result = RollResultType::kSuccess;
            result.result_str = "Успех";
        }
        else {
            result.result = RollResultType::kFail;
            result.result_str = "Неудача";
        }

        return result;
    }

    RollDetails CalculateRoll(
        const std::string& stat_name,
        int difficulty,
        const GameState& state,
        const std::string& context
    ) {
        int stat_value = state.stats.at(stat_name);
        auto result = RollDiceWithModifiers(stat_value, difficulty);

        if (!context.empty()) {
            std::cout << context << ": "
                << result.total_roll << " ("
                << result.result_str << ")\n";
        }

        return result;
    }

    // ===== Condition Evaluation =====
    float EvaluateSimpleExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::string token;
        float result = 0.0f;
        float current = 0.0f;
        char op = '+';

        while (iss >> token) {
            if (token == "+" || token == "-" || token == "*" || token == "/") {
                op = token[0];
            }
            else {
                try {
                    current = std::stof(token);

                    switch (op) {
                    case '+': result += current; break;
                    case '-': result -= current; break;
                    case '*': result *= current; break;
                    case '/':
                        if (std::fabs(current) > 1e-5) result /= current;
                        break;
                    default: break;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Ошибка вычисления выражения: " << e.what() << "\n";
                }
            }
        }

        return result;
    }


    // ===== Game Effects =====
    void ApplyGameEffects(const json& effects, GameState& state) {
        // Установка флагов
        if (effects.contains("set_flags")) {
            for (const auto& [flag, value] : effects["set_flags"].items()) {
                state.flags[flag] = value.get<bool>();
            }
        }

        // Добавление предметов
        if (effects.contains("add_items")) {
            for (const auto& item : effects["add_items"]) {
                state.inventory.push_back(item.get<std::string>());
            }
        }

        // Разблокировка концовок
        if (effects.contains("unlock_ending")) {
            state.unlocked_endings.insert(
                effects["unlock_ending"].get<std::string>()
            );
        }

        // Модификация характеристик
        if (effects.contains("modify_stats")) {
            for (const auto& [stat, value] : effects["modify_stats"].items()) {
                state.stats[stat] += value.get<int>();
            }
        }
    }
  

    // ===== Damage Calculation =====
    int CalculateDamage(const std::string& dice_formula) {
        size_t d_pos = dice_formula.find('d');
        if (d_pos == std::string::npos) {
            // Просто число без кубиков
            return std::stoi(dice_formula);
        }

        int count = 1;
        if (d_pos > 0) {
            count = std::stoi(dice_formula.substr(0, d_pos));
        }

        size_t modifier_pos = dice_formula.find_first_of("+-", d_pos + 1);
        int sides = 0;
        int modifier = 0;

        if (modifier_pos != std::string::npos) {
            sides = std::stoi(dice_formula.substr(d_pos + 1, modifier_pos - d_pos - 1));
            modifier = std::stoi(dice_formula.substr(modifier_pos));
        }
        else {
            sides = std::stoi(dice_formula.substr(d_pos + 1));
        }

        // Бросок кубиков
        int total = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, sides);

        for (int i = 0; i < count; ++i) {
            total += dist(gen);
        }

        return total + modifier;
    }

    // ===== Input Handling =====
    int Input::GetInt(int min, int max) {
        while (true) {
            std::string input;
            std::getline(std::cin, input);

            try {
                int value = std::stoi(input);
                if (value < min || value > max) {
                    std::cout << "Введите значение между " << min << " и " << max << ": ";
                    continue;
                }
                return value;
            }
            catch (const std::exception& e) {
                std::cout << "Неверный ввод. Пожалуйста, введите целое число: ";
            }
        }
    }


    std::string Input::GetLine() {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    RollDetails ResolveAttack(
        const json& attack,
        const std::unordered_map<std::string, int>& attacker_stats) {
        int base_value = attacker_stats.at(attack["stat"].get<std::string>());
        int difficulty = attack.value("difficulty", 0);

        return RollDiceWithModifiers(base_value, difficulty);
    }

    float ParseFormula(const std::string& formula, const std::unordered_map<std::string, int>& stats) {
        // Простая замена переменных значениями
        std::string processed = formula;
        for (const auto& [stat, value] : stats) {
            size_t pos = 0;
            while ((pos = processed.find(stat, pos)) != std::string::npos) {
                processed.replace(pos, stat.length(), std::to_string(value));
                pos += std::to_string(value).length();
            }
        }

        // Теперь вычисляем простое математическое выражение
        std::istringstream iss(processed);
        std::string token;
        float result = 0;
        float current = 0;
        char op = '+';

        while (iss >> token) {
            if (token == "+" || token == "-" || token == "*" || token == "/") {
                op = token[0];
            }
            else {
                try {
                    current = std::stof(token);

                    switch (op) {
                    case '+': result += current; break;
                    case '-': result -= current; break;
                    case '*': result *= current; break;
                    case '/':
                        if (std::fabs(current) > 1e-5) result /= current;
                        break;
                    default: break;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "ERROR parsing token '" << token << "' in formula: " << formula << "\n";
                }
            }
        }

        return result;
    }

    void CalculateDerivedStats(GameState& state, const nlohmann::json& character_base) {
        if (!character_base.contains("derived_stats_formulas")) {
            return;
        }

        const auto& formulas = character_base["derived_stats_formulas"];

        for (const auto& [derived_stat, formula] : formulas.items()) {
            try {
                std::string formula_str = formula.get<std::string>();
                std::string processed = formula_str;

                // Заменяем названия характеристик на их значения
                for (const auto& [stat, value] : state.stats) {
                    size_t pos = 0;
                    while ((pos = processed.find(stat, pos)) != std::string::npos) {
                        processed.replace(pos, stat.length(), std::to_string(value));
                        pos += std::to_string(value).length();
                    }
                }

                // Вычисляем результат
                float value = EvaluateSimpleExpression(processed);
                state.derived_stats[derived_stat] = value;

                std::cout << "Рассчитано: " << derived_stat << " = "
                    << value << " по формуле: " << formula_str
                    << " (вычислено: " << processed << ")\n";
            }
            catch (const std::exception& e) {
                std::cerr << "Ошибка расчета характеристики '" << derived_stat << "': " << e.what() << "\n";
            }
        }
    }

    bool EvaluateCondition(const std::string& condition, const GameState& state) {
        // Простая реализация для проверки флагов и инвентаря
        if (condition.empty()) {
            return true; // Пустое условие всегда истинно
        }

        // Проверка формата: "flag:имя_флага" или "item:имя_предмета"
        size_t pos = condition.find(':');
        if (pos == std::string::npos) {
            std::cerr << "Invalid condition format: " << condition << "\n";
            return false;
        }

        std::string type = condition.substr(0, pos);
        std::string value = condition.substr(pos + 1);

        if (type == "flag") {
            // Проверка наличия флага
            return state.flags.count(value) > 0 && state.flags.at(value);
        }
        else if (type == "item") {
            // Проверка наличия предмета в инвентаре
            return std::find(state.inventory.begin(), state.inventory.end(), value) != state.inventory.end();
        }
        else if (type == "stat") {
            // Проверка характеристик в формате "stat:название>=значение"
            size_t cmp_pos = value.find_first_of("><=");
            if (cmp_pos == std::string::npos) {
                std::cerr << "Invalid stat condition: " << value << "\n";
                return false;
            }

            std::string stat_name = value.substr(0, cmp_pos);
            char op = value[cmp_pos];
            int required_value = std::stoi(value.substr(cmp_pos + 1));

            if (state.stats.count(stat_name)) {
                int stat_value = state.stats.at(stat_name);
                switch (op) {
                case '>': return stat_value > required_value;
                case '<': return stat_value < required_value;
                case '=': return stat_value == required_value;
                default: break;
                }
            }


            return false;
        }

        std::cerr << "Unknown condition type: " << type << "\n";
        return false;
    }

}  // namespace rpg_utils