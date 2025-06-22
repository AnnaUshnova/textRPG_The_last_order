#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>
#include <iostream>
#include <limits>
#include <fstream>

namespace rpg_utils {

    // String utilities

    std::vector<std::string> Split(const std::string& str, char delimiter) {
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

    std::string Trim(const std::string& str) {
        size_t start = 0;
        size_t end = str.size();

        // Trim from start
        while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
            start++;
        }

        // Trim from end
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
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

    // Game utilities

    int RollDice(int count, int sides) {
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

    bool EvaluateCondition(const std::string& condition,
        const std::unordered_map<std::string, bool>& flags) {
        if (condition.empty()) return true;

        std::string trimmed = Trim(condition);

        // Handle complex conditions with logical operators
        if (trimmed.find("&&") != std::string::npos) {
            auto parts = Split(trimmed, '&');
            for (const auto& part : parts) {
                if (!EvaluateCondition(Trim(part), flags)) return false;
            }
            return true;
        }

        if (trimmed.find("||") != std::string::npos) {
            auto parts = Split(trimmed, '|');
            for (const auto& part : parts) {
                if (EvaluateCondition(Trim(part), flags)) return true;
            }
            return false;
        }

        // Handle negation
        if (trimmed[0] == '!') {
            std::string flag = Trim(trimmed.substr(1));
            return !(flags.find(flag) != flags.end() && flags.at(flag));
        }

        // Simple flag check
        return flags.find(trimmed) != flags.end() && flags.at(trimmed);
    }

    void ApplyEffects(const nlohmann::json& effects, GameState& state) {
        if (effects.is_null()) return;

#ifdef DEBUG
        std::cout << "Applying effects: " << effects.dump() << "\n";
#endif

        // ��������� set_flags
        if (effects.contains("set_flags")) {
            const auto& flags = effects["set_flags"];
            if (flags.is_object()) {
                for (auto it = flags.begin(); it != flags.end(); ++it) {
                    state.flags[it.key()] = it.value().get<bool>();
                }
            }
        }

        // ��������� add_items
        if (effects.contains("add_items") && effects["add_items"].is_array()) {
            for (const auto& item : effects["add_items"]) {
                state.inventory.push_back(item.get<std::string>());
            }
        }

        // ��������� remove_items
        if (effects.contains("remove_items") && effects["remove_items"].is_array()) {
            for (const auto& item : effects["remove_items"]) {
                auto it = std::find(state.inventory.begin(), state.inventory.end(),
                    item.get<std::string>());
                if (it != state.inventory.end()) {
                    state.inventory.erase(it);
                }
            }
        }
    }

    int CalculateStat(const std::string& stat_name,
        const std::unordered_map<std::string, int>& stats,
        const DataManager& data) {
        // �������� ������� �������������
        if (stats.find(stat_name) != stats.end()) {
            return stats.at(stat_name);
        }

        try {
            const auto& char_base = data.Get("character_base");
            if (!char_base.contains("derived_stats")) return 0;

            const auto& derived = char_base["derived_stats"];
            if (!derived.contains(stat_name)) return 0;

            const std::string& formula = derived[stat_name].get<std::string>();
            auto tokens = Split(Trim(formula), ' ');

            if (tokens.size() != 3) return 0;

            if (tokens[1] == "*") {
                int base_value = CalculateStat(tokens[0], stats, data);
                int multiplier = std::stoi(tokens[2]);
                return base_value * multiplier;
            }
            else if (tokens[1] == "+") {
                int base_value = CalculateStat(tokens[0], stats, data);
                int add_value = std::stoi(tokens[2]);
                return base_value + add_value;
            }
        }
        catch (const std::exception& e) {
            // ����������� ������ � �������� ������
#ifdef DEBUG
            std::cerr << "Stat calculation error: " << e.what() << std::endl;
#endif
        }

        return 0;
    }

    // Input handling

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

}  // namespace rpg_utils