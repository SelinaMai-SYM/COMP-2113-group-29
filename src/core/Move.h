#pragma once

#include <string>
#include <vector>

/**
 * What it does:
 * Represents one legal move in the puzzle.
 * Inputs:
 * None.
 * Outputs:
 * A plain data structure describing source, target and move size.
 */
struct Move {
    int src = -1;
    int dst = -1;
    int amount = 0;
    int score = 0;
    std::string explain;
};

/**
 * What it does:
 * Stores the previous tube contents for one regular move so undo can restore it.
 * Inputs:
 * None.
 * Outputs:
 * A plain data structure used by the undo power-up.
 */
struct MoveRecord {
    int src = -1;
    int dst = -1;
    int amount = 0;
    std::vector<char> oldSrcSlots;
    std::vector<char> oldDstSlots;
};
