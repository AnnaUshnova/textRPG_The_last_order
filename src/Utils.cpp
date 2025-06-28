#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <Windows.h>

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

        while (start < end && std::isspace(str[start])) {
            start++;
        }
        while (end > start && std::isspace(str[end - 1])) {
            end--;
        }

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
        std::uniform_int_distribution<int> d6(1, 6);

        int roll1 = d6(gen);
        int roll2 = d6(gen);
        int roll3 = d6(gen);
        int total_roll = roll1 + roll2 + roll3;

        // Правильный расчет эффективного навыка
        int effective_skill = skill_value + difficulty_modifier;

        RollResultType result;

        // Критические успехи
        if (total_roll <= 4) {
            result = RollResultType::kCriticalSuccess;
        }
        // Критические провалы
        else if (total_roll >= 17) {
            result = RollResultType::kCriticalFail;
        }
        // Успех при броске <= эффективного навыка
        else if (total_roll <= effective_skill) {
            result = RollResultType::kSuccess;
        }
        // Провал
        else {
            result = RollResultType::kFail;
        }

        return { total_roll, result };
    }

    int CalculateDamage(const std::string& damage_str) {
        // Парсинг строки формата "NdX+M" или "NdX"
        int dice_count = 0;
        int dice_sides = 0;
        int bonus = 0;
        char delimiter;

        std::stringstream ss(damage_str);
        if (ss >> dice_count >> delimiter >> dice_sides) {
            // Пытаемся прочитать бонус, если есть
            std::string bonus_str;
            if (ss >> bonus_str) {
                try {
                    bonus = std::stoi(bonus_str);
                }
                catch (...) {
                    bonus = 0;
                }
            }
        }
        else {
            // Если формат не NdX, пробуем прочитать как число
            try {
                return std::stoi(damage_str);
            }
            catch (...) {
                return 0;
            }
        }

        // Бросок костей
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> die(1, dice_sides);

        int total = 0;
        for (int i = 0; i < dice_count; i++) {
            total += die(gen);
        }

        return total + bonus;
    }

    int Input::GetInt(int min, int max) {
        int value;
        while (true) {
            SetGreenText();  // Зеленый цвет для приглашения ввода
            std::cout << "> ";
            ResetConsoleColor();

            std::string input;
            std::getline(std::cin, input);

            try {
                value = std::stoi(input);
                if (value >= min && value <= max) {
                    return value;
                }
                SetGreenText();
                std::cout << "Введите число от " << min << " до " << max << ".\n";
                ResetConsoleColor();
            }
            catch (...) {
                SetGreenText();
                std::cout << "Некорректный ввод. Введите число.\n";
                ResetConsoleColor();
            }
        }
    }

    std::string Input::GetLine() {
        SetGreenText();  // Зеленый цвет для приглашения ввода
        std::cout << "> ";
        ResetConsoleColor();

        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    // Реализация функций для работы с цветом
    void SetConsoleColor(int color) {
        HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_console, color);
    }

    void ResetConsoleColor() {
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    void SetWhiteText() {
        SetConsoleColor(FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN |
            FOREGROUND_BLUE);
    }

    void SetGreenText() {
        SetConsoleColor(FOREGROUND_INTENSITY | FOREGROUND_GREEN);
    }

}  // namespace rpg_utils