#include "features/PowerUp.h"

#include <algorithm>
#include <sstream>

std::string ReversePowerUp::name() const {
    return "reverse";
}

std::string ReversePowerUp::description() const {
    return "Reverse the order of one incomplete tube.";
}

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

    std::reverse(tube.slots().begin(), tube.slots().end());
    std::ostringstream out;
    out << "Reversed tube " << (index + 1) << ".";
    message = out.str();
    return true;
}
