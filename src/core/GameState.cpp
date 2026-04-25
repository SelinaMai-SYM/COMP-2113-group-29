#include "core/GameState.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

}  // namespace

std::string difficultyToString(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Easy:
            return "easy";
        case Difficulty::Normal:
            return "normal";
        case Difficulty::Hard:
            return "hard";
    }
    return "normal";
}

std::string difficultyLabel(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Easy:
            return "Easy";
        case Difficulty::Normal:
            return "Normal";
        case Difficulty::Hard:
            return "Hard";
    }
    return "Normal";
}

bool parseDifficulty(const std::string& text, Difficulty& difficulty) {
    const std::string lowered = toLowerCopy(text);
    if (lowered == "easy" || lowered == "1") {
        difficulty = Difficulty::Easy;
        return true;
    }
    if (lowered == "normal" || lowered == "2") {
        difficulty = Difficulty::Normal;
        return true;
    }
    if (lowered == "hard" || lowered == "3") {
        difficulty = Difficulty::Hard;
        return true;
    }
    return false;
}

std::string inventorySummary(const Inventory& inventory) {
    std::ostringstream out;
    out << "bin: " << inventory.bin << " | undo: " << inventory.undo
        << " | straw: " << inventory.straw << " | reverse: " << inventory.reverse;
    return out.str();
}
