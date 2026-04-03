#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/GameState.h"

/**
 * What it does:
 * Centralizes the core puzzle rules, move validation, move application and hint logic.
 * Inputs:
 * The current game state plus optional move parameters.
 * Outputs:
 * Legal-move data, solved checks, or state mutations with status messages.
 */
class RuleEngine {
public:
    /**
     * What it does:
     * Computes how many blocks can legally be poured from one tube to another.
     * Inputs:
     * The current game state and two 0-based tube indices.
     * Outputs:
     * The number of movable top blocks, or 0 if the move is illegal.
     */
    static int legalPourAmount(const GameState& state, int src, int dst);

    /**
     * What it does:
     * Enumerates every currently legal regular move.
     * Inputs:
     * The current game state.
     * Outputs:
     * A vector of legal moves.
     */
    static std::vector<Move> enumerateMoves(const GameState& state);

    /**
     * What it does:
     * Applies one regular move to the current state.
     * Inputs:
     * The current state, the move to apply, and whether undo history should be recorded.
     * Outputs:
     * True on success, while updating the state and status message.
     */
    static bool applyMove(GameState& state, const Move& move, bool trackHistory,
                          std::string& message);

    /**
     * What it does:
     * Checks whether the current board satisfies the win condition.
     * Inputs:
     * The current game state.
     * Outputs:
     * True if the level is solved.
     */
    static bool isSolved(const GameState& state);

    /**
     * What it does:
     * Generates a lightweight heuristic hint for the player.
     * Inputs:
     * The current game state.
     * Outputs:
     * The recommended move, or empty if no legal move exists.
     */
    static std::optional<Move> suggestMove(const GameState& state);

    /**
     * What it does:
     * Counts how many tubes currently contain mixed colors.
     * Inputs:
     * The current game state.
     * Outputs:
     * The number of mixed tubes.
     */
    static int countMixedTubes(const GameState& state);
};
