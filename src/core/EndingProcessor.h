#ifndef RPG_ENDING_PROCESSOR_H_
#define RPG_ENDING_PROCESSOR_H_

#include "DataManager.h"
#include "GameState.h"
#include <string>

class EndingProcessor {
public:
    EndingProcessor(DataManager& data, GameState& state);

    // ��������� ���������� ��������
    void Process(const std::string& ending_id);

    // ����� ������ � ���������� ��������
    void ShowEndingCollection();

    // ���������� ��������� (��������)
    void SaveProgress();

private:
    // ����������� ����� ��������
    void DisplayEnding(const nlohmann::json& ending);

    // �������� �������������� �� ��������
    bool IsEndingUnlocked(const std::string& ending_id) const;

    DataManager& data_;
    GameState& state_;
};

#endif  // RPG_ENDING_PROCESSOR_H_