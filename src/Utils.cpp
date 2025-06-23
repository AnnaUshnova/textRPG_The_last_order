#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <iostream>

namespace rpg_utils {

    std::vector<std::string> Split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream token_stream(str);

        while (std::getline(token_stream, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    std::string Trim(const std::string& str) {
        size_t start = 0;
        size_t end = str.length();

        while (start < end && std::isspace(str[start])) start++;
        while (end > start && std::isspace(str[end - 1])) end--;

        return str.substr(start, end - start);
    }

    std::string ToLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    RollDetails RollDiceWithModifiers(int skill_value, int difficulty_modifier) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // Бросок 3d6 как в GURPS
        std::uniform_int_distribution<int> d6(1, 6);
        int roll1 = d6(gen);
        int roll2 = d6(gen);
        int roll3 = d6(gen);
        int total_roll = roll1 + roll2 + roll3;

        // Эффективное значение навыка с учетом модификаторов
        int effective_skill = std::max(1, skill_value + difficulty_modifier);

        // Определение результата
        RollResultType result;

        if (total_roll <= 4) {
            // Критический успех (3-4)
            result = RollResultType::kCriticalSuccess;
        }
        else if (total_roll == 17 || total_roll == 18) {
            // Критическая неудача (17-18)
            result = RollResultType::kCriticalFail;
        }
        else if (total_roll <= effective_skill) {
            // Обычный успех
            result = RollResultType::kSuccess;
        }
        else {
            // Обычная неудача
            result = RollResultType::kFail;
        }

        // Специальные случаи для высоких навыков
        if (effective_skill >= 16) {
            if (total_roll == 5 || total_roll == 6) {
                result = RollResultType::kCriticalSuccess;
            }
        }
        else if (effective_skill <= 15) {
            if (total_roll == 16) {
                result = RollResultType::kCriticalFail;
            }
        }

        return { total_roll, result };
    }

    // Упрощенный расчет урона в стиле GURPS
    int CalculateDamage(const std::string& damage_type) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> d6(1, 6);

        if (damage_type == "light") {
            return d6(gen);  // Легкое оружие: 1d6
        }
        else if (damage_type == "medium") {
            return d6(gen) + d6(gen);  // Среднее оружие: 2d6
        }
        else if (damage_type == "heavy") {
            return d6(gen) + d6(gen) + d6(gen);  // Тяжелое оружие: 3d6
        }
        else if (damage_type == "piercing") {
            return d6(gen) + std::max(0, d6(gen) - 2);  // Колющее: 1d6-2
        }

        return 0;  // Неизвестный тип урона
    }

    // Input implementation
    int Input::GetInt(int min, int max) {
        int value;
        while (true) {
            std::cout << "> ";
            std::string input;
            std::getline(std::cin, input);

            try {
                value = std::stoi(input);
                if (value >= min && value <= max) return value;
                std::cout << "Введите число от " << min << " до " << max << ".\n";
            }
            catch (...) {
                std::cout << "Некорректный ввод. Введите число.\n";
            }
        }
    }

    std::string Input::GetLine() {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        return input;
    }

}  // namespace rpg_utils