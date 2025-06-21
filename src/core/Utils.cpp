#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>
#include <iostream>
#include <limits>

// Реализация методов класса Utils

std::vector<std::string> Utils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

std::string Utils::Trim(const std::string& str) {
    auto start = str.begin();
    auto end = str.end();

    // Удаляем пробелы в начале
    while (start != end && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }

    // Удаляем пробелы в конце
    do {
        --end;
    } while (std::distance(start, end) > 0 &&
        std::isspace(static_cast<unsigned char>(*end)));

    return std::string(start, end + 1);
}

int Utils::RollDice(int count, int sides) {
    if (count <= 0 || sides <= 0) return 0;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, sides);

    int result = 0;
    for (int i = 0; i < count; ++i) {
        result += dist(gen);
    }

    return result;
}

// Реализация методов класса Input

int Input::GetInt(int min, int max) {
    int value;
    while (true) {
        std::cout << "> ";
        std::cin >> value;

        if (std::cin.fail() || value < min || value > max) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number between "
                << min << " and " << max << ".\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

std::string Input::GetLine() {
    std::string input;
    std::cout << "> ";
    std::getline(std::cin, input);
    return input;
}