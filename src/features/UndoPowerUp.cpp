#include "features/PowerUp.h"

#include <algorithm>
#include <sstream>

#include "core/Mechanics.h"

/*
- What it does:
  Returns the command name for the undo power-up.
- Inputs:
  None.
- Outputs:
  The stable identifier string "undo".
*/
std::string UndoPowerUp::name() const {
    return "undo";
}

/*
- What it does:
  Describes the effect of the undo power-up.
- Inputs:
  None.
- Outputs:
  A short player-facing description string.
*/
std::string UndoPowerUp::description() const {
    return "Undo the last regular move on the current level.";
}

/*
- What it does:
  Restores the most recent regular move snapshot from the level history.
- Inputs:
  The current game state, no integer arguments and an output message.
- Outputs:
  True if one move is undone successfully.
*/
bool UndoPowerUp::apply(GameState& state, const std::vector<int>& args, std::string& message) const {
    if (!args.empty()) {
        message = "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }
    if (state.history.empty()) {
        message = "No history to undo";
        return false;
    }

    const MoveRecord record = state.history.back();
    state.history.pop_back();
    syncMechanicState(state);
    state.tubes[record.src].slots() = record.oldSrcSlots;
    state.tubes[record.dst].slots() = record.oldDstSlots;
    state.mechanics.hiddenBlocks[record.src] = record.oldSrcHidden;
    state.mechanics.hiddenBlocks[record.dst] = record.oldDstHidden;
    if (!record.oldObscuredTubes.empty()) {
        state.mechanics.obscuredTubes = record.oldObscuredTubes;
    } else {
        state.mechanics.obscuredTubes[record.src] = record.oldSrcObscured;
        state.mechanics.obscuredTubes[record.dst] = record.oldDstObscured;
    }
    revealExposedBlocks(state);
    state.moves = std::max(0, state.moves - 1);
    state.totalMoves = std::max(0, state.totalMoves - 1);

    std::ostringstream out;
    out << "Undid the last regular move: " << (record.dst + 1) << " -> " << (record.src + 1)
        << ".";
    message = out.str();
    return true;
}
