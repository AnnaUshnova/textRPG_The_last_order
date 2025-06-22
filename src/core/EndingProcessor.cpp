#include "EndingProcessor.h"
#include "Utils.h"
#include <iostream>
#include <fstream>

EndingProcessor::EndingProcessor(DataManager& data, GameState& state)
    : data_(data), state_(state) {
}

void EndingProcessor::Process(const std::string& ending_id) {
    // Загружаем данные концовки
    const auto& ending = data_.Get("endings", ending_id);

    // Показываем концовку
    ShowEnding(ending);

    // Добавляем в коллекцию открытых концовок
    state_.unlocked_endings.insert(ending_id);

    // Показываем коллекцию концовок
    ShowEndingCollection();

    // Сохраняем прогресс
    SaveProgress();

    // Обновляем состояние игры
    state_.active_type = "menu";
}

void EndingProcessor::ShowEnding(const nlohmann::json& ending) {
    std::cout << "\n\n=== " << ending["title"].get<std::string>() << " ===\n";
    std::cout << ending["text"].get<std::string>() << "\n";

    if (ending.contains("achievement")) {
        std::cout << "\n[Достижение: " << ending["achievement"].get<std::string>() << "]\n";
    }

    std::cout << "\nНажмите Enter, чтобы продолжить...";
    rpg_utils::Input::GetLine();
}

void EndingProcessor::ShowEndingCollection() {
    std::cout << "\n\n=== КОЛЛЕКЦИЯ КОНЦОВОК ===\n";
    std::cout << "Открыто: " << state_.unlocked_endings.size() << " из "
        << data_.Get("endings").size() << "\n\n";

    const auto& all_endings = data_.Get("endings");
    size_t counter = 1;

    for (const auto& [id, ending] : all_endings.items()) {
        std::cout << counter++ << ". ";

        if (state_.unlocked_endings.find(id) != state_.unlocked_endings.end()) {
            // Открытая концовка
            std::cout << ending["title"].get<std::string>() << " - ";
            std::cout << ending["text"].get<std::string>() << "\n";
        }
        else {
            // Неоткрытая концовка
            std::cout << "??? (Неизвестная концовка)\n";
        }
    }

    std::cout << "\nНажмите Enter, чтобы вернуться в меню...";
    rpg_utils::Input::GetLine();
}

void EndingProcessor::SaveProgress() {
    try {
        // Обновляем данные о концовках в GameState
        nlohmann::json& game_state = data_.GetMutable("game_state");

        // Создаем массив открытых концовок
        nlohmann::json unlocked_array = nlohmann::json::array();
        for (const auto& ending_id : state_.unlocked_endings) {
            unlocked_array.push_back(ending_id);
        }

        // Обновляем данные
        game_state["unlocked_endings"] = unlocked_array;

        // Сохраняем обратно в файл
        data_.Save("game_state");
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка сохранения прогресса: " << e.what() << std::endl;
    }
}