#include "core/LevelGenerator.h"

#include <algorithm>
#include <random>

namespace {

struct ReverseSplitMove {
    int src = -1;
    int dst = -1;
    int amount = 0;
};

/**
 * What it does:
 * Applies one reverse-scramble move that can be undone by a legal forward move.
 * Inputs:
 * The working state and the chosen reverse move.
 * Outputs:
 * The state with the split applied.
 */
void applyReverseSplit(GameState& state, const ReverseSplitMove& move) {
    Tube& source = state.tubes[move.src];
    Tube& target = state.tubes[move.dst];

    const auto splitPoint = source.slots().end() - move.amount;
    const std::vector<char> moved(splitPoint, source.slots().end());
    source.slots().erase(splitPoint, source.slots().end());
    target.slots().insert(target.slots().end(), moved.begin(), moved.end());
}

/**
 * What it does:
 * Counts how many tubes currently contain mixed colors.
 * Inputs:
 * The working state.
 * Outputs:
 * The number of non-uniform non-empty tubes.
 */
int countMixedTubes(const GameState& state) {
    int mixed = 0;
    for (const Tube& tube : state.tubes) {
        if (!tube.isEmpty() && !tube.isUniform()) {
            ++mixed;
        }
    }
    return mixed;
}

/**
 * What it does:
 * Builds all valid reverse-scramble moves for the current state.
 * Inputs:
 * The working state.
 * Outputs:
 * A vector of candidate reverse moves.
 */
std::vector<ReverseSplitMove> buildReverseMoves(const GameState& state) {
    std::vector<ReverseSplitMove> moves;
    for (int src = 0; src < static_cast<int>(state.tubes.size()); ++src) {
        const Tube& source = state.tubes[src];
        if (source.isEmpty()) {
            continue;
        }

        const int maxRun = source.topRunLength();
        for (int dst = 0; dst < static_cast<int>(state.tubes.size()); ++dst) {
            if (src == dst) {
                continue;
            }

            const Tube& target = state.tubes[dst];
            const int maxAmount = std::min(maxRun, target.space());
            for (int amount = 1; amount <= maxAmount; ++amount) {
                const bool preservesSolvedSwap =
                    source.isUniformFull() && target.isEmpty() && amount == source.height();
                if (!preservesSolvedSwap) {
                    moves.push_back(ReverseSplitMove{src, dst, amount});
                }
            }
        }
    }
    return moves;
}

/**
 * What it does:
 * Creates a solved state before it is scrambled into a playable puzzle.
 * Inputs:
 * The chosen blueprint, palette subset, difficulty and seeds.
 * Outputs:
 * A solved state ready for reverse scrambling.
 */
GameState buildSolvedState(const LevelBlueprint& blueprint, const std::vector<char>& colors,
                           Difficulty difficulty, unsigned int baseSeed,
                           unsigned int currentLevelSeed, int level, int totalLevels) {
    GameState state;
    state.capacity = blueprint.capacity;
    state.expectedBlocks = blueprint.capacity * static_cast<int>(colors.size());
    state.level = level;
    state.totalLevels = totalLevels;
    state.difficulty = difficulty;
    state.colors = colors;
    state.baseSeed = baseSeed;
    state.currentLevelSeed = currentLevelSeed;

    for (char color : colors) {
        state.tubes.emplace_back(blueprint.capacity, std::vector<char>(blueprint.capacity, color));
    }
    for (int count = 0; count < blueprint.extraEmpty; ++count) {
        state.tubes.emplace_back(blueprint.capacity);
    }

    return state;
}

/**
 * What it does:
 * Scrambles a solved state until it becomes a non-trivial, still-solvable puzzle.
 * Inputs:
 * The solved state and a random generator.
 * Outputs:
 * A scrambled game state.
 */
GameState scrambleState(GameState state, const LevelBlueprint& blueprint, std::mt19937& rng) {
    ReverseSplitMove previous{};
    bool hasPrevious = false;

    for (int step = 0; step < blueprint.scrambleSteps; ++step) {
        std::vector<ReverseSplitMove> candidates = buildReverseMoves(state);
        if (candidates.empty()) {
            break;
        }

        if (hasPrevious) {
            candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                            [&](const ReverseSplitMove& move) {
                                                return move.src == previous.dst &&
                                                       move.dst == previous.src &&
                                                       move.amount == previous.amount;
                                            }),
                             candidates.end());
        }
        if (candidates.empty()) {
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
        const ReverseSplitMove chosen = candidates[pick(rng)];
        applyReverseSplit(state, chosen);
        previous = chosen;
        hasPrevious = true;
    }

    return state;
}

}  // namespace

LevelBlueprint LevelGenerator::describeLevel(Difficulty difficulty, int level, int totalLevels) {
    (void)totalLevels;

    LevelBlueprint blueprint;
    switch (difficulty) {
        case Difficulty::Easy:
            blueprint.capacity = 4 + (level - 1) / 5;
            blueprint.colorCount = std::min(6, 3 + (level + 1) / 3);
            blueprint.extraEmpty = 3;
            blueprint.scrambleSteps = 18 + level * 4;
            break;
        case Difficulty::Normal:
            blueprint.capacity = 4 + (level - 1) / 3;
            blueprint.colorCount = std::min(8, 3 + level);
            blueprint.extraEmpty = 2;
            blueprint.scrambleSteps = 24 + level * 5;
            break;
        case Difficulty::Hard:
            blueprint.capacity = 4 + (level - 1) / 3;
            blueprint.colorCount = std::min(8, 4 + level);
            blueprint.extraEmpty = (level >= 6) ? 1 : 2;
            blueprint.scrambleSteps = 30 + level * 6;
            break;
    }
    return blueprint;
}

unsigned int LevelGenerator::computeLevelSeed(unsigned int baseSeed, Difficulty difficulty,
                                              int level) {
    const unsigned int difficultySalt =
        (difficulty == Difficulty::Easy) ? 101U : (difficulty == Difficulty::Normal ? 211U : 307U);
    return baseSeed + difficultySalt * static_cast<unsigned int>(level * 97);
}

GameState LevelGenerator::buildLevel(int level, Difficulty difficulty, unsigned int baseSeed,
                                     int totalLevels) {
    const LevelBlueprint blueprint = describeLevel(difficulty, level, totalLevels);
    const unsigned int seed = computeLevelSeed(baseSeed, difficulty, level);
    std::mt19937 rng(seed);

    std::vector<char> colors = palette();
    colors.resize(blueprint.colorCount);

    GameState state = buildSolvedState(blueprint, colors, difficulty, baseSeed, seed, level,
                                       totalLevels);

    for (int attempt = 0; attempt < 12; ++attempt) {
        GameState scrambled = scrambleState(state, blueprint, rng);
        if (countMixedTubes(scrambled) >= 2) {
            scrambled.message = "Level " + std::to_string(level) + " ready. Good luck!";
            return scrambled;
        }
        rng.seed(seed + static_cast<unsigned int>(attempt + 1));
    }

    state.message = "Level " + std::to_string(level) + " ready. Good luck!";
    return state;
}

std::vector<char> LevelGenerator::palette() {
    return {'R', 'G', 'B', 'Y', 'O', 'P', 'C', 'M'};
}
