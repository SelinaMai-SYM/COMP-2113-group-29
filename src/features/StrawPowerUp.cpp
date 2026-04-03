#include "features/PowerUp.h"

#include <sstream>

std::string StrawPowerUp::name() const {
    return "straw";
}

std::string StrawPowerUp::description() const {
    return "Move the bottom block from one tube to the top of another.";
}

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

    const char bottom = source.slots().front();
    source.slots().erase(source.slots().begin());
    target.slots().push_back(bottom);

    std::ostringstream out;
    out << "Moved the bottom block from tube " << (src + 1) << " to tube " << (dst + 1)
        << ".";
    message = out.str();
    return true;
}
