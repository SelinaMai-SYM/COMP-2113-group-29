#include "core/GameController.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

namespace {

std::string trimCopy(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

}  // namespace

GameController::GameController() = default;

void GameController::run() {
    printWelcome();

    while (running_) {
        std::cout << "\nMain menu: [n] new game, [q] quit\n> ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nInput closed. Exiting.\n";
            return;
        }

        input = toLowerCopy(trimCopy(input));
        if (input == "q" || input == "quit") {
            running_ = false;
            std::cout << "Goodbye.\n";
            continue;
        }

        if (input == "n" || input == "new") {
            Difficulty difficulty = Difficulty::Normal;
            if (chooseDifficulty(difficulty)) {
                startNewGame(difficulty);
            }
            continue;
        }

        std::cout << "Unknown option. Choose n or q.\n";
    }
}

bool GameController::chooseDifficulty(Difficulty& difficulty) const {
    std::cout << "Choose difficulty [1 easy, 2 normal, 3 hard, q cancel]\n> ";

    std::string input;
    if (!std::getline(std::cin, input)) {
        return false;
    }

    input = trimCopy(input);
    if (toLowerCopy(input) == "q") {
        return false;
    }

    if (!parseDifficulty(input, difficulty)) {
        std::cout << "Invalid difficulty. Defaulting to Normal.\n";
        difficulty = Difficulty::Normal;
    }
    return true;
}

void GameController::startNewGame(Difficulty difficulty) {
    GameState state = buildStageOneDemoState(difficulty);
    state.message = "Stage 1 foundation: the controller skeleton is active.";
    playGame(state);
}

void GameController::playGame(GameState state) const {
    printState(state);
    std::cout << "\nThis stage only sets up the core foundation.\n";
    std::cout << "Press Enter to return to the main menu.";

    std::string ignored;
    std::getline(std::cin, ignored);
}

GameState GameController::buildStageOneDemoState(Difficulty difficulty) const {
    GameState state;
    state.difficulty = difficulty;
    state.tubes = {
        Tube(4, {'R', 'B'}),
        Tube(4, {'B', 'R'}),
        Tube(4),
        Tube(4),
    };
    return state;
}

void GameController::printWelcome() const {
    std::cout << "Colorful Tube - Mai stage 1 core foundation\n";
}

void GameController::printState(const GameState& state) const {
    std::cout << "\nDifficulty: " << difficultyLabel(state.difficulty) << "\n";
    std::cout << "Level: " << state.level << "/" << state.totalLevels << "\n";
    std::cout << "Inventory: " << inventorySummary(state.inventory) << "\n";
    std::cout << "Message: " << state.message << "\n";
    std::cout << "Board:\n";

    for (std::size_t index = 0; index < state.tubes.size(); ++index) {
        std::cout << "  Tube " << (index + 1) << ": " << state.tubes[index].serialize() << "\n";
    }
}
