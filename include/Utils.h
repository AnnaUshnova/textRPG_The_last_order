// Utils.h
#ifndef RPG_UTILS_H_
#define RPG_UTILS_H_

#include <string>
#include <vector>

#define NOMINMAX
#include <windows.h>

namespace rpg_utils {

	std::vector<std::string> Split(const std::string& str, char delimiter);
	std::string Trim(const std::string& str);
	std::string ToLower(const std::string& str);

	enum class RollResultType {
		kCriticalSuccess,
		kSuccess,
		kFail,
		kCriticalFail
	};

	struct RollDetails {
		int total_roll;
		RollResultType result;
	};

	RollDetails RollDiceWithModifiers(int base_value, int modifier);
	int CalculateDamage(const std::string& dice_formula);

	// Color functions
	void SetConsoleColor(int color);
	void ResetConsoleColor();
	void SetWhiteText();
	void SetGreenText();

	class Input {
	public:
		static int GetInt(int min, int max);
		static std::string GetLine();
	};

}  // namespace rpg_utils

#endif  // RPG_UTILS_H_