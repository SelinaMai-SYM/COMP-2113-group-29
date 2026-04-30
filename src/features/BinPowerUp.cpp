#include "features/PowerUp.h"

#include <sstream>

#include "core/Mechanics.h"

/*
- What it does:
  Returns the command name for the bin power-up.
- Inputs:
  None.
- Outputs:
  The stable identifier string "bin".
*/
std::string BinPowerUp::name() const {
    return "bin";
}

/*
- What it does:
  Describes the effect of the bin power-up.
- Inputs:
  None.
- Outputs:
  A short player-facing description string.
*/
std::string BinPowerUp::description() const {
    return "Delete the top block from a non-empty tube.";
}

/*
- What it does:
  Deletes the top block from one chosen tube and updates mechanic state.
- Inputs:
  The current game state, one 0-based tube index argument and an output message.
- Outputs:
  True if the block is removed successfully.
*/
bool BinPowerUp::apply(GameState& state, const std::vector<int>& args, std::string& message) const {
    if (args.size() != 1) {
        message = "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }

    const int index = args[0];
    if (index < 0 || index >= static_cast<int>(state.tubes.size())) {
        message = "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }
    if (isTubeObscured(state, index)) {
        message = "Tube " + std::to_string(index + 1) + " is still covered. Complete one full " +
                  colorLabel(obscuredTargetColor(state, index)) + " tube to unlock it first.";
        return false;
    }

    Tube& tube = state.tubes[index];
    if (tube.isEmpty()) {
        message = "That tube is empty. Try another input!";
        return false;
    }

    syncMechanicState(state);
    const int hiddenBefore = hiddenBlockCount(state);
    int unlockedTubeIndex = -1;
    tube.slots().pop_back();
    if (!state.mechanics.hiddenBlocks[static_cast<std::size_t>(index)].empty()) {
        state.mechanics.hiddenBlocks[static_cast<std::size_t>(index)].pop_back();
    }
    state.removedBlocksByBin += 1;
    unlockCoveredTubesIfTargetMet(state, &unlockedTubeIndex);
    revealExposedBlocks(state);
    const int hiddenAfter = hiddenBlockCount(state);

    std::ostringstream out;
    out << "Deleted the top block from tube " << (index + 1) << ".";
    if (unlockedTubeIndex >= 0) {
        out << " The white cover on tube " << (unlockedTubeIndex + 1)
            << " unlocked after the " << colorLabel(obscuredTargetColor(state, unlockedTubeIndex))
            << " target tube was completed.";
    }
    if (hiddenBefore > hiddenAfter) {
        out << " An unknown color surfaced.";
    }
    message = out.str();
    return true;
}
