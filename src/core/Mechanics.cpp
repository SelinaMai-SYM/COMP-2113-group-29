#include "core/Mechanics.h"

#include <algorithm>

namespace {

/*
- What it does:
  Checks whether a tube index is valid for the current board.
- Inputs:
  The current game state and one 0-based tube index.
- Outputs:
  True if the tube exists.
*/
bool isValidTube(const GameState& state, int tubeIndex) {
    return tubeIndex >= 0 && tubeIndex < static_cast<int>(state.tubes.size());
}

/*
- What it does:
  Checks whether one completed tube matches a target color exactly.
- Inputs:
  The current game state and one target color code.
- Outputs:
  True if a full uniform tube of that color already exists.
*/
bool hasCompletedTargetTube(const GameState& state, char targetColor) {
    const char canonical = canonicalColorCode(targetColor);
    for (const Tube& tube : state.tubes) {
        if (tube.isUniformFull() && canonicalColorCode(tube.topColor()) == canonical) {
            return true;
        }
    }
    return false;
}

}  // namespace

/*
- What it does:
  Resizes all mechanic-tracking arrays so they match the current board.
- Inputs:
  One mutable game state.
- Outputs:
  The mechanic metadata is synchronized with the tube layout.
*/
void syncMechanicState(GameState& state) {
    state.mechanics.hiddenBlocks.resize(state.tubes.size());
    state.mechanics.obscuredTubes.resize(state.tubes.size(), false);
    state.mechanics.obscuredUnlockTargets.resize(state.tubes.size(), '.');

    for (std::size_t index = 0; index < state.tubes.size(); ++index) {
        state.mechanics.hiddenBlocks[index].resize(state.tubes[index].height(), false);
    }

    if (state.mechanics.initialHiddenBlocks == 0) {
        state.mechanics.initialHiddenBlocks = hiddenBlockCount(state);
    }
    if (state.mechanics.initialObscuredTubes == 0) {
        state.mechanics.initialObscuredTubes = obscuredTubeCount(state);
    }

    revealExposedBlocks(state);
}

/*
- What it does:
  Reveals any hidden block that is currently exposed at the top of a visible tube.
- Inputs:
  One mutable game state.
- Outputs:
  Topmost exposed hidden blocks become visible to the player.
*/
void revealExposedBlocks(GameState& state) {
    for (std::size_t index = 0; index < state.tubes.size(); ++index) {
        if (index >= state.mechanics.hiddenBlocks.size() || index >= state.mechanics.obscuredTubes.size()) {
            continue;
        }
        if (state.mechanics.obscuredTubes[index] || state.mechanics.hiddenBlocks[index].empty()) {
            continue;
        }
        state.mechanics.hiddenBlocks[index].back() = false;
    }
}

/*
- What it does:
  Counts how many blocks are still hidden by the unknown-color mechanic.
- Inputs:
  The current game state.
- Outputs:
  The total number of hidden blocks remaining.
*/
int hiddenBlockCount(const GameState& state) {
    int count = 0;
    for (const std::vector<bool>& flags : state.mechanics.hiddenBlocks) {
        for (bool hidden : flags) {
            if (hidden) {
                ++count;
            }
        }
    }
    return count;
}

/*
- What it does:
  Counts how many tubes are still completely obscured.
- Inputs:
  The current game state.
- Outputs:
  The number of whiteout tubes not yet opened.
*/
int obscuredTubeCount(const GameState& state) {
    int count = 0;
    for (bool obscured : state.mechanics.obscuredTubes) {
        if (obscured) {
            ++count;
        }
    }
    return count;
}

/*
- What it does:
  Checks whether one tube is still covered by the obscured-tube mechanic.
- Inputs:
  The current game state and one 0-based tube index.
- Outputs:
  True if the tube contents should remain hidden.
*/
bool isTubeObscured(const GameState& state, int tubeIndex) {
    if (!isValidTube(state, tubeIndex) ||
        tubeIndex >= static_cast<int>(state.mechanics.obscuredTubes.size())) {
        return false;
    }
    return state.mechanics.obscuredTubes[static_cast<std::size_t>(tubeIndex)];
}

/*
- What it does:
  Returns the target color that unlocks one covered tube.
- Inputs:
  The current game state and one 0-based tube index.
- Outputs:
  The target color code, or '.' when that tube has no target.
*/
char obscuredTargetColor(const GameState& state, int tubeIndex) {
    if (!isValidTube(state, tubeIndex) ||
        tubeIndex >= static_cast<int>(state.mechanics.obscuredUnlockTargets.size())) {
        return '.';
    }
    return canonicalColorCode(
        state.mechanics.obscuredUnlockTargets[static_cast<std::size_t>(tubeIndex)]);
}

/*
- What it does:
  Returns the character that should be rendered for one block position.
- Inputs:
  The current state, one tube index and one bottom-based slot index.
- Outputs:
  The visible block character, which may differ from the true color.
*/
char visibleBlockAt(const GameState& state, int tubeIndex, int slotIndex) {
    if (!isValidTube(state, tubeIndex)) {
        return '.';
    }

    const Tube& tube = state.tubes[static_cast<std::size_t>(tubeIndex)];
    if (slotIndex < 0 || slotIndex >= tube.height()) {
        return '.';
    }

    if (isTubeObscured(state, tubeIndex)) {
        return '~';
    }

    if (tubeIndex < static_cast<int>(state.mechanics.hiddenBlocks.size()) &&
        slotIndex < static_cast<int>(state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)].size()) &&
        state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)][static_cast<std::size_t>(slotIndex)]) {
        return '?';
    }

    return canonicalColorCode(tube.slots()[static_cast<std::size_t>(slotIndex)]);
}

/*
- What it does:
  Removes hidden-state flags from the top of one tube during a move.
- Inputs:
  One mutable game state, one tube index and the number of moved blocks.
- Outputs:
  The moved hidden flags in bottom-to-top order, or an empty vector on failure.
*/
std::vector<bool> removeTopHiddenFlags(GameState& state, int tubeIndex, int amount) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex) || amount <= 0) {
        return {};
    }

    std::vector<bool>& flags = state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)];
    if (amount > static_cast<int>(flags.size())) {
        return {};
    }

    std::vector<bool> moved;
    const int start = static_cast<int>(flags.size()) - amount;
    moved.reserve(static_cast<std::size_t>(amount));
    for (int index = start; index < static_cast<int>(flags.size()); ++index) {
        moved.push_back(flags[static_cast<std::size_t>(index)]);
    }
    flags.erase(flags.end() - amount, flags.end());
    return moved;
}

/*
- What it does:
  Appends moved hidden-state flags to the top of one destination tube.
- Inputs:
  One mutable game state, one tube index and the flags to append.
- Outputs:
  The destination hidden-state array gains the provided flags.
*/
void appendTopHiddenFlags(GameState& state, int tubeIndex, const std::vector<bool>& flags) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return;
    }

    std::vector<bool>& hidden = state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)];
    hidden.insert(hidden.end(), flags.begin(), flags.end());
}

/*
- What it does:
  Removes the hidden flag associated with the bottom block of one tube.
- Inputs:
  One mutable game state, one tube index and one output flag reference.
- Outputs:
  True on success after writing the removed hidden flag into the output parameter.
*/
bool removeBottomHiddenFlag(GameState& state, int tubeIndex, bool& hidden) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return false;
    }

    std::vector<bool>& flags = state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)];
    if (flags.empty()) {
        return false;
    }

    hidden = flags.front();
    flags.erase(flags.begin());
    return true;
}

/*
- What it does:
  Appends one hidden-state flag to the top of a tube.
- Inputs:
  One mutable game state, one tube index and the hidden flag value.
- Outputs:
  The destination tube gains one extra hidden-state entry.
*/
void appendTopHiddenFlag(GameState& state, int tubeIndex, bool hidden) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return;
    }
    state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)].push_back(hidden);
}

/*
- What it does:
  Reverses the hidden-state order of one tube after a block-order reversal.
- Inputs:
  One mutable game state and one tube index.
- Outputs:
  The hidden flags for that tube are reversed in place.
*/
void reverseHiddenFlags(GameState& state, int tubeIndex) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return;
    }
    std::vector<bool>& flags = state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)];
    std::reverse(flags.begin(), flags.end());
    revealExposedBlocks(state);
}

/*
- What it does:
  Removes the obscured-tube mask from one tube after it is opened.
- Inputs:
  One mutable game state and one tube index.
- Outputs:
  True if the tube was previously obscured and is now revealed.
*/
bool liftObscuredTube(GameState& state, int tubeIndex) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return false;
    }

    if (!state.mechanics.obscuredTubes[static_cast<std::size_t>(tubeIndex)]) {
        return false;
    }
    state.mechanics.obscuredTubes[static_cast<std::size_t>(tubeIndex)] = false;
    revealExposedBlocks(state);
    return true;
}

/*
- What it does:
  Unlocks any covered tube whose target color has already been completed elsewhere.
- Inputs:
  One mutable game state and an optional output pointer for the unlocked tube index.
- Outputs:
  True if one covered tube is unlocked successfully.
*/
bool unlockCoveredTubesIfTargetMet(GameState& state, int* unlockedTubeIndex) {
    syncMechanicState(state);
    for (std::size_t index = 0; index < state.tubes.size(); ++index) {
        if (!state.mechanics.obscuredTubes[index]) {
            continue;
        }
        const char targetColor = obscuredTargetColor(state, static_cast<int>(index));
        if (targetColor == '.' || !hasCompletedTargetTube(state, targetColor)) {
            continue;
        }
        state.mechanics.obscuredTubes[index] = false;
        revealExposedBlocks(state);
        if (unlockedTubeIndex != nullptr) {
            *unlockedTubeIndex = static_cast<int>(index);
        }
        return true;
    }
    return false;
}

/*
- What it does:
  Marks one existing block as initially hidden by the unknown-color mechanic.
- Inputs:
  One mutable game state, one tube index and one bottom-based slot index.
- Outputs:
  True if the target block was marked successfully.
*/
bool markHiddenBlock(GameState& state, int tubeIndex, int slotIndex) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return false;
    }

    std::vector<bool>& flags = state.mechanics.hiddenBlocks[static_cast<std::size_t>(tubeIndex)];
    if (slotIndex < 0 || slotIndex >= static_cast<int>(flags.size())) {
        return false;
    }
    flags[static_cast<std::size_t>(slotIndex)] = true;
    return true;
}

/*
- What it does:
  Marks one tube as initially obscured by the whiteout mechanic.
- Inputs:
  One mutable game state and one tube index.
- Outputs:
  True if the tube was marked successfully.
*/
bool markObscuredTube(GameState& state, int tubeIndex, char targetColor) {
    syncMechanicState(state);
    if (!isValidTube(state, tubeIndex)) {
        return false;
    }
    state.mechanics.obscuredTubes[static_cast<std::size_t>(tubeIndex)] = true;
    state.mechanics.obscuredUnlockTargets[static_cast<std::size_t>(tubeIndex)] =
        canonicalColorCode(targetColor);
    return true;
}
