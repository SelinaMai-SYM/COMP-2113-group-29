#include "features/PowerUp.h"

#include <sstream>

#include "core/Mechanics.h"

/*
- What it does:
  Returns the command name for the straw power-up.
- Inputs:
  None.
- Outputs:
  The stable identifier string "straw".
*/
std::string StrawPowerUp::name() const {
    return "straw";
}

/*
- What it does:
  Describes the effect of the straw power-up.
- Inputs:
  None.
- Outputs:
  A short player-facing description string.
*/
std::string StrawPowerUp::description() const {
    return "Move the bottom block from one tube to the top of another.";
}

/*
- What it does:
  Moves the bottom block from one tube onto the top of another tube.
- Inputs:
  The current game state, two 0-based tube index arguments and an output message.
- Outputs:
  True if the bottom-block transfer succeeds.
*/
bool StrawPowerUp::apply(GameState& state, const std::vector<int>& args, std::string& message) const {
    if (args.size() != 2) {
        message = "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }

    const int src = args[0];
    const int dst = args[1];
    if (src < 0 || dst < 0 || src >= static_cast<int>(state.tubes.size()) ||
        dst >= static_cast<int>(state.tubes.size()) || src == dst) {
        message = "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }
    if (isTubeObscured(state, src)) {
        message = "Tube " + std::to_string(src + 1) + " is still covered. Complete one full " +
                  colorLabel(obscuredTargetColor(state, src)) + " tube to unlock it first.";
        return false;
    }
    if (isTubeObscured(state, dst)) {
        message = "Tube " + std::to_string(dst + 1) + " is still covered. Complete one full " +
                  colorLabel(obscuredTargetColor(state, dst)) + " tube to unlock it first.";
        return false;
    }

    Tube& source = state.tubes[src];
    Tube& target = state.tubes[dst];
    if (source.isEmpty()) {
        message = "The source tube is empty. Try another input!";
        return false;
    }
    if (target.space() <= 0) {
        message = "The target tube is already full. Try another input!";
        return false;
    }

    syncMechanicState(state);
    const int hiddenBefore = hiddenBlockCount(state);
    int unlockedTubeIndex = -1;
    const char bottom = source.slots().front();
    bool bottomHidden = false;
    removeBottomHiddenFlag(state, src, bottomHidden);
    source.slots().erase(source.slots().begin());
    target.slots().push_back(bottom);
    appendTopHiddenFlag(state, dst, bottomHidden);
    unlockCoveredTubesIfTargetMet(state, &unlockedTubeIndex);
    revealExposedBlocks(state);
    const int hiddenAfter = hiddenBlockCount(state);

    std::ostringstream out;
    out << "Moved the bottom block from tube " << (src + 1) << " to tube " << (dst + 1)
        << ".";
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
