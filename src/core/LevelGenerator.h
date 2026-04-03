#pragma once

#include <vector>

#include "core/GameState.h"

/**
 * What it does:
 * Describes the parameters used to generate one level.
 * Inputs:
 * None.
 * Outputs:
 * Capacity, color count, empty tubes and scramble depth for a level.
 */
struct LevelBlueprint {
    int capacity = 4;
    int colorCount = 4;
    int extraEmpty = 2;
    int scrambleSteps = 30;
};

/**
 * What it does:
 * Creates deterministic, solvable levels from a base seed and difficulty.
 * Inputs:
 * A level index, difficulty, and base seed.
 * Outputs:
 * A fresh game state for that level.
 */
class LevelGenerator {
public:
    /**
     * What it does:
     * Returns the generation blueprint for a given difficulty and level.
     * Inputs:
     * Difficulty, level number and total level count.
     * Outputs:
     * A configured blueprint for level generation.
     */
    static LevelBlueprint describeLevel(Difficulty difficulty, int level, int totalLevels = 15);

    /**
     * What it does:
     * Computes a deterministic seed for one level.
     * Inputs:
     * A run-wide base seed, the difficulty and the level number.
     * Outputs:
     * The seed that should be used for this level.
     */
    static unsigned int computeLevelSeed(unsigned int baseSeed, Difficulty difficulty, int level);

    /**
     * What it does:
     * Builds a fresh, solvable level state.
     * Inputs:
     * Level number, difficulty, run seed and total level count.
     * Outputs:
     * A ready-to-play game state.
     */
    static GameState buildLevel(int level, Difficulty difficulty, unsigned int baseSeed,
                                int totalLevels = 15);

    /**
     * What it does:
     * Returns the palette of supported block colors.
     * Inputs:
     * None.
     * Outputs:
     * A vector of color identifiers used by rendering and generation.
     */
    static std::vector<char> palette();
};
