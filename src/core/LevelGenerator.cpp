#include "core/LevelGenerator.h"

#include <algorithm>
#include <random>

#include "core/Mechanics.h"
#include "core/RuleEngine.h"

namespace {

struct ReverseSplitMove {
    int src = -1;
    int dst = -1;
    int amount = 0;
};

struct CoveredCandidate {
    int tubeIndex = -1;
    char targetColor = '.';
};

constexpr int kGenerationAttempts = 48;

/*
- What it does:
  Applies one reverse-scramble move that can be undone by a legal forward move.
- Inputs:
  The working state and the chosen reverse move.
- Outputs:
  The state with the split applied.
*/
void applyReverseSplit(GameState& state, const ReverseSplitMove& move) {
    Tube& source = state.tubes[move.src];
    Tube& target = state.tubes[move.dst];

    const auto splitPoint = source.slots().end() - move.amount;
    const std::vector<char> moved(splitPoint, source.slots().end());
    source.slots().erase(splitPoint, source.slots().end());
    target.slots().insert(target.slots().end(), moved.begin(), moved.end());
}

/*
- What it does:
  Counts how many tubes currently contain mixed colors.
- Inputs:
  The working state.
- Outputs:
  The number of non-uniform non-empty tubes.
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

/*
- What it does:
  Checks whether a freshly generated board is safe to start playing immediately.
- Inputs:
  The generated game state.
- Outputs:
  True if the opening board is unsolved and not deadlocked.
*/
bool isPlayableOpeningState(const GameState& state) {
    return !RuleEngine::isSolved(state) && !RuleEngine::isDeadlocked(state);
}

/*
- What it does:
  Scores one generated opening state so fallback selection prefers richer boards.
- Inputs:
  One generated game state.
- Outputs:
  A larger score for states with more mixing and more successfully applied mechanics.
*/
int openingStateScore(const GameState& state) {
    return countMixedTubes(state) * 1000 + state.mechanics.initialObscuredTubes * 100 +
           state.mechanics.initialHiddenBlocks * 10;
}

/*
- What it does:
  Builds candidate positions for hidden-color blocks below the visible top layer.
- Inputs:
  The current scrambled state.
- Outputs:
  A list of tube/slot coordinates that may be hidden safely.
*/
std::vector<std::pair<int, int>> buildHiddenCandidates(const GameState& state) {
    std::vector<std::pair<int, int>> candidates;
    for (int tubeIndex = 0; tubeIndex < static_cast<int>(state.tubes.size()); ++tubeIndex) {
        const Tube& tube = state.tubes[static_cast<std::size_t>(tubeIndex)];
        for (int slotIndex = 0; slotIndex < tube.height() - 1; ++slotIndex) {
            candidates.push_back({tubeIndex, slotIndex});
        }
    }
    return candidates;
}

/*
- What it does:
  Checks whether one tube currently contains a specific color.
- Inputs:
  One tube object and one target color code.
- Outputs:
  True if the tube contains that color anywhere.
*/
bool tubeContainsColor(const Tube& tube, char color) {
    const char canonical = canonicalColorCode(color);
    return std::any_of(tube.slots().begin(), tube.slots().end(), [&](char block) {
        return canonicalColorCode(block) == canonical;
    });
}

/*
- What it does:
  Checks whether a full uniform tube of one color already exists on the board.
- Inputs:
  The current state and one target color code.
- Outputs:
  True if that color is already completed.
*/
bool hasCompletedColorTube(const GameState& state, char color) {
    const char canonical = canonicalColorCode(color);
    for (const Tube& tube : state.tubes) {
        if (tube.isUniformFull() && canonicalColorCode(tube.topColor()) == canonical) {
            return true;
        }
    }
    return false;
}

/*
- What it does:
  Builds candidate covered-tube and target-color pairs for the white-cover mechanic.
- Inputs:
  The current scrambled state.
- Outputs:
  A vector of valid tube/target pairs that keep the puzzle unlockable.
*/
std::vector<CoveredCandidate> buildCoveredCandidates(const GameState& state) {
    std::vector<CoveredCandidate> candidates;
    for (int tubeIndex = 0; tubeIndex < static_cast<int>(state.tubes.size()); ++tubeIndex) {
        const Tube& tube = state.tubes[static_cast<std::size_t>(tubeIndex)];
        if (!tube.isEmpty() && !tube.isUniformFull()) {
            for (char color : state.colors) {
                const char targetColor = canonicalColorCode(color);
                if (tubeContainsColor(tube, targetColor) || hasCompletedColorTube(state, targetColor)) {
                    continue;
                }
                candidates.push_back(CoveredCandidate{tubeIndex, targetColor});
            }
        }
    }
    return candidates;
}

/*
- What it does:
  Applies the unknown-color and obscured-tube mechanics to a freshly generated board.
- Inputs:
  The mutable game state, level blueprint and random generator.
- Outputs:
  The state gains mechanic metadata while keeping the puzzle solvable.
*/
bool applySpecialMechanics(GameState& state, const LevelBlueprint& blueprint, std::mt19937& rng) {
    syncMechanicState(state);

    std::vector<std::pair<int, int>> hiddenCandidates = buildHiddenCandidates(state);
    std::shuffle(hiddenCandidates.begin(), hiddenCandidates.end(), rng);
    const int hiddenCount =
        std::min(blueprint.hiddenBlockCount, static_cast<int>(hiddenCandidates.size()));
    for (int index = 0; index < hiddenCount; ++index) {
        markHiddenBlock(state, hiddenCandidates[static_cast<std::size_t>(index)].first,
                        hiddenCandidates[static_cast<std::size_t>(index)].second);
    }

    std::vector<CoveredCandidate> coveredCandidates = buildCoveredCandidates(state);
    coveredCandidates.erase(std::remove_if(coveredCandidates.begin(), coveredCandidates.end(),
                                           [&](const CoveredCandidate& candidate) {
                                                const std::vector<bool>& flags =
                                                    state.mechanics.hiddenBlocks
                                                        [static_cast<std::size_t>(candidate.tubeIndex)];
                                                return std::any_of(flags.begin(), flags.end(),
                                                                   [](bool hidden) { return hidden; });
                                            }),
                            coveredCandidates.end());
    std::shuffle(coveredCandidates.begin(), coveredCandidates.end(), rng);
    const int requestedCoveredCount = blueprint.obscuredTubeCount;
    int appliedCoveredCount = 0;
    std::vector<bool> usedTubes(state.tubes.size(), false);
    if (requestedCoveredCount > 0) {
        for (const CoveredCandidate& candidate : coveredCandidates) {
            if (usedTubes[static_cast<std::size_t>(candidate.tubeIndex)]) {
                continue;
            }
            if (markObscuredTube(state, candidate.tubeIndex, candidate.targetColor)) {
                usedTubes[static_cast<std::size_t>(candidate.tubeIndex)] = true;
                ++appliedCoveredCount;
            }
            if (appliedCoveredCount >= requestedCoveredCount) {
                break;
            }
        }
    }

    revealExposedBlocks(state);
    state.mechanics.initialHiddenBlocks = hiddenBlockCount(state);
    state.mechanics.initialObscuredTubes = obscuredTubeCount(state);
    return state.mechanics.initialHiddenBlocks == hiddenCount &&
           state.mechanics.initialObscuredTubes == requestedCoveredCount;
}

/*
- What it does:
  Builds the opening message for a level while mentioning any active special mechanics.
- Inputs:
  The generated state.
- Outputs:
  A short gameplay message shown when the level starts.
*/
std::string buildOpeningMessage(const GameState& state) {
    std::string message = "Level " + std::to_string(state.level) + " ready. ";
    if (state.mechanics.initialHiddenBlocks > 0 || state.mechanics.initialObscuredTubes > 0) {
        message += "Special rules detected:";
        if (state.mechanics.initialHiddenBlocks > 0) {
            message += " " + std::to_string(state.mechanics.initialHiddenBlocks) +
                       " unknown block(s) marked with '?'";
        }
        if (state.mechanics.initialObscuredTubes > 0) {
            if (state.mechanics.initialHiddenBlocks > 0) {
                message += ",";
            }
            int coveredTubeIndex = -1;
            char targetColor = '.';
            for (int index = 0; index < static_cast<int>(state.tubes.size()); ++index) {
                if (!isTubeObscured(state, index)) {
                    continue;
                }
                coveredTubeIndex = index;
                targetColor = obscuredTargetColor(state, index);
                break;
            }
            if (coveredTubeIndex >= 0 && targetColor != '.') {
                message += " tube " + std::to_string(coveredTubeIndex + 1) +
                           " sealed behind a white cover; complete one full " +
                           colorLabel(targetColor) + " tube to unlock it";
            } else {
                message += " " + std::to_string(state.mechanics.initialObscuredTubes) +
                           " obscured tube(s) sealed behind a white cover";
            }
        }
        message += ".";
        return message;
    }
    return message + "Good luck!";
}

/*
- What it does:
  Builds all valid reverse-scramble moves for the current state.
- Inputs:
  The working state.
- Outputs:
  A vector of candidate reverse moves.
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

/*
- What it does:
  Creates a solved state before it is scrambled into a playable puzzle.
- Inputs:
  The chosen blueprint, palette subset, difficulty and seeds.
- Outputs:
  A solved state ready for reverse scrambling.
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

/*
- What it does:
  Scrambles a solved state until it becomes a non-trivial, still-solvable puzzle.
- Inputs:
  The solved state and a random generator.
- Outputs:
  A scrambled game state.
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
            blueprint.hiddenBlockCount = (level >= 5) ? std::min(4, 1 + (level - 5) / 3) : 0;
            blueprint.obscuredTubeCount = (level >= 10) ? 1 : 0;
            break;
        case Difficulty::Normal:
            blueprint.capacity = 4 + (level - 1) / 3;
            blueprint.colorCount = std::min(8, 3 + level);
            blueprint.extraEmpty = 2;
            blueprint.scrambleSteps = 24 + level * 5;
            blueprint.hiddenBlockCount = (level >= 3) ? std::min(5, 1 + (level - 3) / 2) : 0;
            blueprint.obscuredTubeCount = (level >= 7) ? 1 : 0;
            break;
        case Difficulty::Hard:
            blueprint.capacity = 4 + (level - 1) / 3;
            blueprint.colorCount = std::min(8, 4 + level);
            blueprint.extraEmpty = (level >= 6) ? 1 : 2;
            blueprint.scrambleSteps = 30 + level * 6;
            blueprint.hiddenBlockCount = (level >= 2) ? std::min(6, 1 + (level - 2) / 2) : 0;
            blueprint.obscuredTubeCount = (level >= 4) ? 1 : 0;
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
    GameState bestFallback;
    bool hasFallback = false;
    int bestFallbackScore = -1;

    for (int attempt = 0; attempt < kGenerationAttempts; ++attempt) {
        GameState scrambled = scrambleState(state, blueprint, rng);
        if (!isPlayableOpeningState(scrambled)) {
            rng.seed(seed + static_cast<unsigned int>(attempt + 1));
            continue;
        }

        const int mixedTubes = countMixedTubes(scrambled);
        const bool mechanicsComplete = applySpecialMechanics(scrambled, blueprint, rng);
        if (isPlayableOpeningState(scrambled)) {
            const int score = openingStateScore(scrambled);
            if (!hasFallback || score > bestFallbackScore) {
                bestFallback = scrambled;
                bestFallbackScore = score;
                hasFallback = true;
            }
        }

        if (mixedTubes >= 2 && mechanicsComplete) {
            scrambled.message = buildOpeningMessage(scrambled);
            return scrambled;
        }
        rng.seed(seed + static_cast<unsigned int>(attempt + 1));
    }

    if (hasFallback) {
        bestFallback.message = buildOpeningMessage(bestFallback);
        return bestFallback;
    }

    for (int attempt = 0; attempt < kGenerationAttempts; ++attempt) {
        rng.seed(seed + 1000U + static_cast<unsigned int>(attempt));
        GameState emergencyFallback = scrambleState(state, blueprint, rng);
        if (!isPlayableOpeningState(emergencyFallback)) {
            continue;
        }
        applySpecialMechanics(emergencyFallback, blueprint, rng);
        if (!isPlayableOpeningState(emergencyFallback)) {
            continue;
        }
        emergencyFallback.message = buildOpeningMessage(emergencyFallback);
        return emergencyFallback;
    }

    const std::vector<ReverseSplitMove> emergencyMoves = buildReverseMoves(state);
    for (const ReverseSplitMove& move : emergencyMoves) {
        GameState forcedFallback = state;
        applyReverseSplit(forcedFallback, move);
        applySpecialMechanics(forcedFallback, blueprint, rng);
        if (!isPlayableOpeningState(forcedFallback)) {
            continue;
        }
        forcedFallback.message = buildOpeningMessage(forcedFallback);
        return forcedFallback;
    }

    state.message = "Level generation failed to produce a safe opening board.";
    return state;
}

std::vector<char> LevelGenerator::palette() {
    return {'R', 'G', 'B', 'Y', 'W', 'P', 'C', 'M'};
}
