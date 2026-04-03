#include "features/PowerUp.h"

#include <algorithm>
#include <sstream>

std::string UndoPowerUp::name() const {
    return "undo";
}

std::string UndoPowerUp::description() const {
    return "Undo the last regular move on the current level.";
}

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
    state.tubes[record.src].slots() = record.oldSrcSlots;
    state.tubes[record.dst].slots() = record.oldDstSlots;
    state.moves = std::max(0, state.moves - 1);
    state.totalMoves = std::max(0, state.totalMoves - 1);

    std::ostringstream out;
    out << "Undid the last regular move: " << (record.dst + 1) << " -> " << (record.src + 1)
        << ".";
    message = out.str();
    return true;
}
