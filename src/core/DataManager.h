#ifndef RPG_DATA_MANAGER_H_
#define RPG_DATA_MANAGER_H_

#include "json.hpp"
#include <string>
#include <unordered_map>

class DataManager {
public:
    void LoadAll(const std::string& data_dir);

    const nlohmann::json& Get(const std::string& type,
        const std::string& id = "") const;
    nlohmann::json& GetMutable(const std::string& type);

    void Save(const std::string& type);

private:
    std::unordered_map<std::string, nlohmann::json> data_sets_;
    std::string data_path_;
};

#endif  // RPG_DATA_MANAGER_H_