#include "features/PowerUp.h"

#include <algorithm>
#include <sstream>

#include "core/Mechanics.h"

/*
- What it does:
  Returns the command name for the reverse power-up.
- Inputs:
  None.
- Outputs:
  The stable identifier string "reverse".
*/
std::string ReversePowerUp::name() const {
    return "reverse";
}

/*
- What it does:
  Describes the effect of the reverse power-up.
- Inputs:
  None.
- Outputs:
  A short player-facing description string.
*/
std::string ReversePowerUp::description() const {
    return "Reverse the order of one incomplete tube.";
}

/*
- What it does:
  Reverses the block order inside one chosen incomplete tube.
- Inputs:
  The current game state, one 0-based tube index argument and an output message.
- Outputs:
  True if the selected tube can be reversed successfully.
*/
bool ReversePowerUp::apply(GameState& state, const std::vector<int>& args,
                           std::string& message) const {
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
    if (tube.height() < 2) {
        message = "That tube needs at least 2 blocks. Try another input!";
        return false;
    }
    if (tube.isUniform()) {
        message = tube.isUniformFull() ? "That tube is already completed. Try another input!"
                                       : "That tube is already one color. Try another input!";
        return false;
    }

    syncMechanicState(state);
    const int hiddenBefore = hiddenBlockCount(state);
    int unlockedTubeIndex = -1;
    std::reverse(tube.slots().begin(), tube.slots().end());
    reverseHiddenFlags(state, index);
    unlockCoveredTubesIfTargetMet(state, &unlockedTubeIndex);
    const int hiddenAfter = hiddenBlockCount(state);
    std::ostringstream out;
    out << "Reversed tube " << (index + 1) << ".";
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
