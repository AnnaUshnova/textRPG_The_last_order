#include "EndingProcessor.h"
#include "SceneProcessor.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>

using namespace rpg_utils;
using json = nlohmann::json;

EndingProcessor::EndingProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void EndingProcessor::Process(const std::string& ending_id) {
    try {
        const auto& ending = data_.Get("endings", ending_id);
        DisplayEnding(ending);

        // Разблокировка концовки
        state_.unlocked_endings.insert(ending_id);
        SaveProgress();

        // Пауза перед возвратом
        std::cout << "\n\nНажмите Enter, чтобы продолжить...";
        Input::GetLine();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки концовки: " << e.what() << "\n";
    }
}

void EndingProcessor::ShowEndingCollection() {
    const auto& endings = data_.Get("endings");
    int unlocked_count = 0;

    std::cout << "════════════════════════════════════════\n";
    std::cout << "        КОЛЛЕКЦИЯ КОНЦОВОК\n";
    std::cout << "════════════════════════════════════════\n\n";

    for (const auto& [id, ending] : endings.items()) {
        std::cout << "[" << (IsEndingUnlocked(id) ? "X" : " ") << "] ";
        std::cout << ending["title"].get<std::string>() << "\n";

        if (IsEndingUnlocked(id)) {
            unlocked_count++;
        }
    }

    std::cout << "\nРазблокировано: " << unlocked_count
        << " из " << endings.size() << " концовок\n";
    std::cout << "\nВыберите концовку для просмотра (0 - назад): ";

    int choice = Input::GetInt(0, endings.size());
    if (choice > 0) {
        int index = 1;
        for (const auto& [id, ending] : endings.items()) {
            if (index == choice) {
                if (IsEndingUnlocked(id)) {
                    DisplayEnding(ending);
                }
                else {
                    std::cout << "Эта концовка еще не разблокирована!\n";
                }
                break;
            }
            index++;
        }
    }
}

void EndingProcessor::DisplayEnding(const json& ending) {
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "   " << ending["title"].get<std::string>() << "\n";
    std::cout << "════════════════════════════════════════\n\n";

    std::cout << ending["text"].get<std::string>() << "\n\n";

    if (ending.contains("achievement")) {
        std::cout << "Достижение: "
            << ending["achievement"].get<std::string>() << "\n";
    }
}

bool EndingProcessor::IsEndingUnlocked(const std::string& ending_id) const {
    return state_.unlocked_endings.find(ending_id) != state_.unlocked_endings.end();
}

void EndingProcessor::SaveProgress() {
    // Реализация сохранения прогресса в файл
    // (будет добавлена позже)
}