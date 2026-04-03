#include "features/PowerUp.h"

#include <sstream>

std::string BinPowerUp::name() const {
    return "bin";
}

std::string BinPowerUp::description() const {
    return "Delete the top block from a non-empty tube.";
}

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

    Tube& tube = state.tubes[index];
    if (tube.isEmpty()) {
        message = "That tube is empty. Try another input!";
        return false;
    }

    tube.slots().pop_back();
    std::ostringstream out;
    out << "Deleted the top block from tube " << (index + 1) << ".";
    message = out.str();
    return true;
}
