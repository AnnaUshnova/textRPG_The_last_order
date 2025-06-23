#ifndef RPG_DATA_MANAGER_H_
#define RPG_DATA_MANAGER_H_

#include "json.hpp"
#include <string>
#include <unordered_map>

class DataManager {
public:
    // Загрузка всех данных из указанной директории
    void LoadAll(const std::string& data_dir);

    // Доступ к данным по типу
    const nlohmann::json& Get(const std::string& type) const;

    // Доступ к конкретному объекту по типу и ID
    const nlohmann::json& Get(const std::string& type, const std::string& id) const;

    // Доступ к изменяемым данным (для сохранения)
    nlohmann::json& GetMutable(const std::string& type);
    void ValidateData() const;

private:
    // Внутренняя структура хранения данных
    std::unordered_map<std::string, nlohmann::json> data_sets_;
    std::string data_path_;

    // Вспомогательные методы
    void LoadJsonFile(const std::string& filename, const std::string& type);
};

#endif  // RPG_DATA_MANAGER_H_