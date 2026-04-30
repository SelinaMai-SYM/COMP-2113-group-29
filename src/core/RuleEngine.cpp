#include "core/RuleEngine.h"

#include <limits>
#include <map>
#include <sstream>

#include "core/Mechanics.h"

namespace {

/*
- What it does:
  Counts the number of color segments inside one tube.
- Inputs:
  One tube object.
- Outputs:
  The number of color runs from bottom to top.
*/
int segmentCount(const Tube& tube) {
    if (tube.isEmpty()) {
        return 0;
    }

    int segments = 1;
    const std::vector<char>& slots = tube.slots();
    for (std::size_t index = 1; index < slots.size(); ++index) {
        if (slots[index] != slots[index - 1]) {
            ++segments;
        }
    }
    return segments;
}

/*
- What it does:
  Measures how disordered the current board is.
- Inputs:
  The current game state.
- Outputs:
  A lower score for cleaner, more solved boards.
*/
int disorderScore(const GameState& state) {
    int score = 0;
    for (const Tube& tube : state.tubes) {
        score += segmentCount(tube);
        if (!tube.isEmpty() && !tube.isUniform()) {
            score += 3;
        }
    }
    return score;
}

/*
- What it does:
  Builds a short explanation string for a suggested move.
- Inputs:
  The simulated move result and the original move.
- Outputs:
  A short human-readable hint message.
*/
std::string explainHint(const GameState& before, const GameState& simulated, const Move& move) {
    std::ostringstream out;
    out << "try " << (move.src + 1) << " -> " << (move.dst + 1);
    int unlockedTubeIndex = -1;
    for (int index = 0;
         index < static_cast<int>(before.mechanics.obscuredTubes.size()) &&
         index < static_cast<int>(simulated.mechanics.obscuredTubes.size());
         ++index) {
        if (before.mechanics.obscuredTubes[static_cast<std::size_t>(index)] &&
            !simulated.mechanics.obscuredTubes[static_cast<std::size_t>(index)]) {
            unlockedTubeIndex = index;
            break;
        }
    }
    if (unlockedTubeIndex >= 0) {
        out << " to unlock covered tube " << (unlockedTubeIndex + 1) << " by completing the "
            << colorLabel(obscuredTargetColor(simulated, unlockedTubeIndex)) << " target.";
    } else if (hiddenBlockCount(simulated) < hiddenBlockCount(before)) {
        out << " to reveal an unknown color.";
    } else if (simulated.tubes[move.dst].isUniformFull()) {
        out << " to complete tube " << (move.dst + 1) << ".";
    } else if (simulated.tubes[move.src].isEmpty()) {
        out << " to empty tube " << (move.src + 1) << ".";
    } else {
        out << " to reduce the number of mixed tubes.";
    }
    return out.str();
}

/*
- What it does:
  Builds a suffix describing any mechanic reveals triggered by a move.
- Inputs:
  The hidden-block counts before and after the move, plus whether an obscured tube opened.
- Outputs:
  A short suffix that can be appended to the move message.
*/
std::string mechanicRevealSuffix(const std::vector<bool>& obscuredBefore, const GameState& state,
                                 int hiddenBefore, int hiddenAfter) {
    std::ostringstream out;
    for (int index = 0;
         index < static_cast<int>(obscuredBefore.size()) &&
         index < static_cast<int>(state.mechanics.obscuredTubes.size());
         ++index) {
        if (!obscuredBefore[static_cast<std::size_t>(index)] ||
            state.mechanics.obscuredTubes[static_cast<std::size_t>(index)]) {
            continue;
        }
        out << " The white cover on tube " << (index + 1) << " unlocked after the "
            << colorLabel(obscuredTargetColor(state, index)) << " target tube was completed.";
        break;
    }

    const int revealed = hiddenBefore - hiddenAfter;
    if (revealed == 1) {
        out << " One unknown color surfaced.";
    } else if (revealed > 1) {
        out << " " << revealed << " unknown colors surfaced.";
    }
    return out.str();
}

}  // namespace

int RuleEngine::legalPourAmount(const GameState& state, int src, int dst) {
    if (src < 0 || dst < 0 || src >= static_cast<int>(state.tubes.size()) ||
        dst >= static_cast<int>(state.tubes.size()) || src == dst) {
        return 0;
    }
    if (isTubeObscured(state, src) || isTubeObscured(state, dst)) {
        return 0;
    }

    const Tube& source = state.tubes[src];
    const Tube& target = state.tubes[dst];
    if (source.isEmpty() || target.space() <= 0) {
        return 0;
    }

    return std::min(source.topRunLength(), target.space());
}

std::vector<Move> RuleEngine::enumerateMoves(const GameState& state) {
    std::vector<Move> moves;
    for (int src = 0; src < static_cast<int>(state.tubes.size()); ++src) {
        for (int dst = 0; dst < static_cast<int>(state.tubes.size()); ++dst) {
            if (src == dst) {
                continue;
            }
            const int amount = legalPourAmount(state, src, dst);
            if (amount > 0) {
                moves.push_back(Move{src, dst, amount, 0, ""});
            }
        }
    }
    return moves;
}

bool RuleEngine::applyMove(GameState& state, const Move& move, bool trackHistory,
                           std::string& message) {
    const int legalAmount = legalPourAmount(state, move.src, move.dst);
    if (legalAmount == 0 || legalAmount != move.amount) {
        message = "That pour is not legal right now. Use 'hint' if you are stuck.";
        return false;
    }

    syncMechanicState(state);
    Tube& source = state.tubes[move.src];
    Tube& target = state.tubes[move.dst];
    const std::vector<char> oldSource = source.slots();
    const std::vector<char> oldTarget = target.slots();
    const std::vector<bool> oldSourceHidden = state.mechanics.hiddenBlocks[move.src];
    const std::vector<bool> oldTargetHidden = state.mechanics.hiddenBlocks[move.dst];
    const bool oldSourceObscured = isTubeObscured(state, move.src);
    const bool oldTargetObscured = isTubeObscured(state, move.dst);
    const std::vector<bool> oldObscuredTubes = state.mechanics.obscuredTubes;
    const int hiddenBefore = hiddenBlockCount(state);

    const auto splitPoint = source.slots().end() - move.amount;
    const std::vector<char> moved(splitPoint, source.slots().end());
    const std::vector<bool> movedHidden = removeTopHiddenFlags(state, move.src, move.amount);
    source.slots().erase(splitPoint, source.slots().end());
    target.slots().insert(target.slots().end(), moved.begin(), moved.end());
    appendTopHiddenFlags(state, move.dst, movedHidden);
    unlockCoveredTubesIfTargetMet(state);
    revealExposedBlocks(state);
    const int hiddenAfter = hiddenBlockCount(state);

    if (trackHistory) {
        state.moves += 1;
        state.totalMoves += 1;
        state.history.push_back(MoveRecord{move.src, move.dst, move.amount, oldSource, oldTarget,
                                           oldSourceHidden, oldTargetHidden, oldSourceObscured,
                                           oldTargetObscured, oldObscuredTubes});
    }

    std::ostringstream out;
    out << "Poured " << move.amount << " block(s) from tube " << (move.src + 1) << " to tube "
        << (move.dst + 1) << ".";
    message = out.str() + mechanicRevealSuffix(oldObscuredTubes, state, hiddenBefore, hiddenAfter);
    return true;
}

bool RuleEngine::isSolved(const GameState& state) {
    int currentBlocks = 0;
    std::map<char, int> colorCounts;
    std::map<char, int> colorTubeCounts;
    for (char color : state.colors) {
        colorCounts[color] = 0;
        colorTubeCounts[color] = 0;
    }

    for (const Tube& tube : state.tubes) {
        currentBlocks += tube.height();
        if (!tube.isEmpty() && !tube.isUniform()) {
            return false;
        }
        if (!tube.isEmpty()) {
            const char color = tube.slots().front();
            colorCounts[color] += tube.height();
            colorTubeCounts[color] += 1;
        }
    }

    if (currentBlocks > state.expectedBlocks) {
        return false;
    }

    if (currentBlocks < state.expectedBlocks) {
        const int missingBlocks = state.expectedBlocks - currentBlocks;
        if (state.removedBlocksByBin != missingBlocks || missingBlocks <= 0) {
            return false;
        }
        for (char color : state.colors) {
            if (colorTubeCounts[color] > 1) {
                return false;
            }
        }
        return true;
    }

    for (char color : state.colors) {
        if (colorCounts[color] != state.capacity) {
            return false;
        }
    }

    for (const Tube& tube : state.tubes) {
        if (!tube.isEmpty() && !tube.isUniformFull()) {
            return false;
        }
    }
    return true;
}

std::optional<Move> RuleEngine::suggestMove(const GameState& state) {
    const std::vector<Move> moves = enumerateMoves(state);
    if (moves.empty()) {
        return std::nullopt;
    }

    int bestScore = std::numeric_limits<int>::min();
    Move bestMove = moves.front();
    const int beforeDisorder = disorderScore(state);
    const int beforeHidden = hiddenBlockCount(state);

    for (const Move& move : moves) {
        GameState simulated = state;
        std::string ignoredMessage;
        if (!applyMove(simulated, move, false, ignoredMessage)) {
            continue;
        }

        int score = 0;
        const int afterDisorder = disorderScore(simulated);
        score += (beforeDisorder - afterDisorder) * 8;

        if (simulated.tubes[move.dst].isUniformFull()) {
            score += 70;
        }
        if (simulated.tubes[move.src].isEmpty()) {
            score += 15;
        }
        if (state.tubes[move.dst].isEmpty()) {
            score += 4;
        }
        if (obscuredTubeCount(simulated) < obscuredTubeCount(state)) {
            score += 28;
        }
        score += (beforeHidden - hiddenBlockCount(simulated)) * 18;
        score -= countMixedTubes(simulated) * 2;

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            bestMove.score = score;
            bestMove.explain = explainHint(state, simulated, move);
        }
    }

    return bestMove;
}

int RuleEngine::countMixedTubes(const GameState& state) {
    int mixed = 0;
    for (const Tube& tube : state.tubes) {
        if (!tube.isEmpty() && !tube.isUniform()) {
            ++mixed;
        }
    }
    return mixed;
}

bool RuleEngine::isDeadlocked(const GameState& state) {
    return !isSolved(state) && enumerateMoves(state).empty();
}

bool RuleEngine::resolveDeadlock(GameState& state, std::string& message) {
    if (!isDeadlocked(state)) {
        return false;
    }

    state.tubes.emplace_back(state.capacity);
    syncMechanicState(state);
    state.history.clear();
    state.mechanics.deadlocksResolved += 1;
    state.mechanics.bonusEmptyTubesGranted += 1;
    state.mechanics.rewardBlocked = true;
    state.mechanics.deadlockPenaltyMoves += 3;
    state.moves += 3;
    state.totalMoves += 3;

    std::ostringstream out;
    out << "Deadlock detected. The game opened an emergency empty tube, but this level's "
           "lucky reward is now forfeited and 3 penalty moves were added.";
    message = out.str();
    return true;
}
