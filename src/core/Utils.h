#pragma once

#include <string>
#include <vector>
#include <random>

class Utils {
public:
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::string Trim(const std::string& str);
    static int RollDice(int count, int sides);
};

class Input {
public:
    static int GetInt(int min, int max);
    static std::string GetLine();
};