#ifndef RPG_ENDING_PROCESSOR_H_
#define RPG_ENDING_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include <string>

class EndingProcessor {
public:
    EndingProcessor(DataManager& data, GameState& state);

    // Обработка конкретной концовки
    void Process(const std::string& ending_id);

    // Показ экрана с коллекцией концовок
    void ShowEndingCollection();

    // Сохранение прогресса (концовки)
    void SaveProgress();

private:
    // Отображение одной концовки
    void DisplayEnding(const nlohmann::json& ending);

    // Проверка разблокирована ли концовка
    bool IsEndingUnlocked(const std::string& ending_id) const;

    DataManager& data_;
    GameState& state_;
};

#endif  // RPG_ENDING_PROCESSOR_H_