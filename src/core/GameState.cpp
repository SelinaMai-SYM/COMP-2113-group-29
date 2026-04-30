#include "core/GameState.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

/*
- What it does:
  Converts one string to lowercase for case-insensitive parsing.
- Inputs:
  One source string.
- Outputs:
  A lowercase copy of the original text.
*/
std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

}  // namespace

/*
- What it does:
  Converts a difficulty enum into the stable save-file string.
- Inputs:
  One difficulty value.
- Outputs:
  The lowercase difficulty name.
*/
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

/*
- What it does:
  Converts a difficulty enum into the player-facing label.
- Inputs:
  One difficulty value.
- Outputs:
  The formatted difficulty name used in menus and HUD text.
*/
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

/*
- What it does:
  Parses menu or save text into one difficulty enum.
- Inputs:
  The source text and one output difficulty reference.
- Outputs:
  True when parsing succeeds after updating the output parameter.
*/
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

/*
- What it does:
  Formats one compact summary of the current inventory counts.
- Inputs:
  The current inventory structure.
- Outputs:
  One printable line listing all power-up totals.
*/
std::string inventorySummary(const Inventory& inventory) {
    std::ostringstream out;
    out << "bin: " << inventory.bin << " | undo: " << inventory.undo
        << " | straw: " << inventory.straw << " | reverse: " << inventory.reverse;
    return out.str();
}

/*
- What it does:
  Returns the fixed list of supported power-up identifiers.
- Inputs:
  None.
- Outputs:
  A vector containing each power-up name once.
*/
std::vector<std::string> powerUpNames() {
    return {"bin", "undo", "straw", "reverse"};
}

/*
- What it does:
  Normalizes one stored color code so legacy orange saves map onto the new white block.
- Inputs:
  One raw block color character.
- Outputs:
  The canonical color code used by the current game.
*/
char canonicalColorCode(char color) {
    return (color == 'O') ? 'W' : color;
}

/*
- What it does:
  Converts one color code into a readable name for prompts and mechanic text.
- Inputs:
  One canonical or legacy color code.
- Outputs:
  A capitalized color name.
*/
std::string colorLabel(char color) {
    switch (canonicalColorCode(color)) {
        case 'R':
            return "Red";
        case 'G':
            return "Green";
        case 'B':
            return "Blue";
        case 'Y':
            return "Yellow";
        case 'W':
            return "White";
        case 'P':
            return "Purple";
        case 'C':
            return "Cyan";
        case 'M':
            return "Black";
    }
    return "Unknown";
}
