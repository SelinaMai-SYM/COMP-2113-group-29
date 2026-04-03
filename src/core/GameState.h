#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/Move.h"
#include "core/Tube.h"

/**
 * What it does:
 * Enumerates the supported game difficulties.
 * Inputs:
 * None.
 * Outputs:
 * A difficulty value used by level generation and statistics.
 */
enum class Difficulty {
    Easy,
    Normal,
    Hard
};

/**
 * What it does:
 * Stores the player's current power-up inventory.
 * Inputs:
 * None.
 * Outputs:
 * Counts for each power-up.
 */
struct Inventory {
    int bin = 0;
    int undo = 0;
    int straw = 0;
    int reverse = 0;
};

/**
 * What it does:
 * Stores persistent statistics across multiple game sessions.
 * Inputs:
 * None.
 * Outputs:
 * Aggregated counters used by the stats page and save file.
 */
struct StatsRecord {
    int gamesStarted = 0;
    int gamesCleared = 0;
    int totalMovesAcrossWins = 0;
    std::map<std::string, int> powerUpUses;
    std::map<std::string, int> powerUpRewards;
    std::map<Difficulty, int> bestMoves;
};

/**
 * What it does:
 * Holds the complete state of one active run.
 * Inputs:
 * None.
 * Outputs:
 * The live game state needed by rendering, saving and gameplay logic.
 */
struct GameState {
    std::vector<Tube> tubes;
    std::vector<char> colors;
    int capacity = 4;
    int expectedBlocks = 0;
    int level = 1;
    int totalLevels = 15;
    int moves = 0;
    int totalMoves = 0;
    Difficulty difficulty = Difficulty::Normal;
    Inventory inventory;
    std::vector<MoveRecord> history;
    std::map<std::string, int> powerUpsUsed;
    std::map<std::string, int> powerUpRewards;
    unsigned int baseSeed = 0;
    unsigned int currentLevelSeed = 0;
    std::string message;
};

/**
 * What it does:
 * Converts a difficulty enum to a stable save-file string.
 * Inputs:
 * The difficulty enum value.
 * Outputs:
 * A machine-friendly string.
 */
std::string difficultyToString(Difficulty difficulty);

/**
 * What it does:
 * Converts a difficulty enum to a human-friendly label.
 * Inputs:
 * The difficulty enum value.
 * Outputs:
 * A display string for menus and panels.
 */
std::string difficultyLabel(Difficulty difficulty);

/**
 * What it does:
 * Parses a difficulty string from menu or save-file text.
 * Inputs:
 * A string such as "easy", "normal" or "hard".
 * Outputs:
 * True if parsing succeeds, and writes the result into the output parameter.
 */
bool parseDifficulty(const std::string& text, Difficulty& difficulty);

/**
 * What it does:
 * Builds a short one-line inventory summary.
 * Inputs:
 * The current inventory values.
 * Outputs:
 * A formatted summary string.
 */
std::string inventorySummary(const Inventory& inventory);

/**
 * What it does:
 * Returns the fixed list of supported power-up names.
 * Inputs:
 * None.
 * Outputs:
 * A vector containing the four power-up identifiers.
 */
std::vector<std::string> powerUpNames();
