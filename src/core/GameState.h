#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/Move.h"
#include "core/Tube.h"

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

struct Inventory {
    int bin = 0;
    int undo = 0;
    int straw = 0;
    int reverse = 0;
};

struct StatsRecord {
    int gamesStarted = 0;
    int gamesCleared = 0;
    int totalMovesAcrossWins = 0;
    std::map<std::string, int> powerUpUses;
};

struct GameState {
    std::vector<Tube> tubes;
    int capacity = 4;
    int level = 1;
    int totalLevels = 15;
    int moves = 0;
    int totalMoves = 0;
    Difficulty difficulty = Difficulty::Normal;
    Inventory inventory;
    std::vector<MoveRecord> history;
    std::string message;
};

std::string difficultyToString(Difficulty difficulty);
std::string difficultyLabel(Difficulty difficulty);
bool parseDifficulty(const std::string& text, Difficulty& difficulty);
std::string inventorySummary(const Inventory& inventory);
