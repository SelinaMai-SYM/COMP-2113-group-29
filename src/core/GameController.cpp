#include "core/GameController.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>

#include "core/LevelGenerator.h"
#include "core/RuleEngine.h"

namespace {

/**
 * What it does:
 * Converts a string to lowercase.
 * Inputs:
 * One text string.
 * Outputs:
 * The lowered copy.
 */
std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

/**
 * What it does:
 * Trims whitespace from both ends of a string.
 * Inputs:
 * One text string.
 * Outputs:
 * The trimmed copy.
 */
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

/**
 * What it does:
 * Returns a mutable inventory counter by power-up name.
 * Inputs:
 * The current inventory and power-up name.
 * Outputs:
 * A pointer to the matching counter, or nullptr if the name is unknown.
 */
int* inventoryCounter(Inventory& inventory, const std::string& name) {
    if (name == "bin") {
        return &inventory.bin;
    }
    if (name == "undo") {
        return &inventory.undo;
    }
    if (name == "straw") {
        return &inventory.straw;
    }
    if (name == "reverse") {
        return &inventory.reverse;
    }
    return nullptr;
}

/**
 * What it does:
 * Increments the rewarded inventory item.
 * Inputs:
 * The current inventory and the rewarded power-up name.
 * Outputs:
 * None.
 */
void grantReward(Inventory& inventory, const std::string& name) {
    if (int* counter = inventoryCounter(inventory, name)) {
        *counter += 1;
    }
}

/**
 * What it does:
 * Builds a short success suffix showing the remaining count of one power-up.
 * Inputs:
 * The power-up name and the remaining amount.
 * Outputs:
 * A human-readable suffix string.
 */
std::string remainingPowerUpText(const std::string& name, int remaining) {
    return " Remaining " + name + ": " + std::to_string(remaining) + ".";
}

/**
 * What it does:
 * Ensures that every tracked power-up key exists in a string-to-count map.
 * Inputs:
 * One mutable power-up counter map.
 * Outputs:
 * Missing keys are inserted with zero values.
 */
void ensureTrackedCounts(std::map<std::string, int>& counts) {
    for (const std::string& name : powerUpNames()) {
        counts.try_emplace(name, 0);
    }
}

/**
 * What it does:
 * Resets per-run power-up tracking maps on a fresh game.
 * Inputs:
 * One mutable game state.
 * Outputs:
 * All per-run usage and reward counters become zero.
 */
void resetRunTracking(GameState& state) {
    for (const std::string& name : powerUpNames()) {
        state.powerUpsUsed[name] = 0;
        state.powerUpRewards[name] = 0;
    }
}

/**
 * What it does:
 * Ensures that loaded or rebuilt states contain all run-tracking counters.
 * Inputs:
 * One mutable game state.
 * Outputs:
 * Missing run-tracking keys are inserted with zero values.
 */
void ensureRunTracking(GameState& state) {
    ensureTrackedCounts(state.powerUpsUsed);
    ensureTrackedCounts(state.powerUpRewards);
}

struct GameScreenGuard {
    const Renderer& renderer;

    ~GameScreenGuard() {
        renderer.leaveGameScreen();
    }
};

}  // namespace

GameController::GameController() : stats_(saveManager_.loadStats()) {
    ensureTrackedCounts(stats_.powerUpUses);
    ensureTrackedCounts(stats_.powerUpRewards);
}

void GameController::run() {
    while (running_) {
        renderer_.renderMenu(stats_, saveManager_.hasSave());

        std::string input;
        if (!std::getline(std::cin, input)) {
            running_ = false;
            break;
        }

        const std::string choice = toLowerCopy(trimCopy(input));
        if (choice == "s" || choice == "start") {
            Difficulty difficulty = Difficulty::Normal;
            if (chooseDifficulty(difficulty)) {
                startNewGame(difficulty);
            }
        } else if (choice == "load") {
            loadSavedGame();
        } else if (choice == "h" || choice == "help") {
            renderer_.renderHelp();
            renderer_.waitForEnter("Press Enter to return to menu...");
        } else if (choice == "stats") {
            renderer_.renderStats(stats_);
            renderer_.waitForEnter("Press Enter to return to menu...");
        } else if (choice == "q" || choice == "quit") {
            running_ = false;
        } else {
            std::cout << "Invalid option. Enter S/H/Q\n";
            renderer_.waitForEnter();
        }
    }

    renderer_.leaveGameScreen();
    std::cout << "\nThanks for playing!\n";
}

void GameController::startNewGame(Difficulty difficulty) {
    GameState state = LevelGenerator::buildLevel(1, difficulty, SaveManager::defaultSeed());
    resetRunTracking(state);
    state.message.clear();

    stats_.gamesStarted += 1;
    persistStats();
    playGame(state);
}

void GameController::loadSavedGame() {
    std::string error;
    std::optional<GameState> state = saveManager_.loadGame(error);
    if (!state.has_value()) {
        std::cout << "\n" << error << "\n";
        renderer_.waitForEnter("Press Enter to return to menu...");
        return;
    }
    ensureRunTracking(*state);
    state->message.clear();
    playGame(*state);
}

void GameController::playGame(GameState state) {
    GameScreenGuard gameScreenGuard{renderer_};
    detailsPanelExpanded_ = false;
    bool returnToMenu = false;
    while (running_ && !returnToMenu) {
        if (RuleEngine::isSolved(state)) {
            advanceAfterLevelClear(state);
            if (!running_) {
                return;
            }
            if (state.level > state.totalLevels) {
                return;
            }
            continue;
        }

        renderer_.renderGame(state, saveManager_.hasSave(), detailsPanelExpanded_);
        std::string line;
        if (!std::getline(std::cin, line)) {
            running_ = false;
            return;
        }

        handleGameCommand(state, parser_.parse(line), returnToMenu);
    }
}

bool GameController::chooseDifficulty(Difficulty& difficulty) {
    while (running_) {
        renderer_.renderDifficultyMenu();
        std::string input;
        if (!std::getline(std::cin, input)) {
            running_ = false;
            return false;
        }

        const std::string choice = toLowerCopy(trimCopy(input));
        if (choice == "b" || choice == "back") {
            return false;
        }
        if (parseDifficulty(choice, difficulty)) {
            return true;
        }
    }
    return false;
}

void GameController::handleGameCommand(GameState& state, const Command& command,
                                       bool& returnToMenu) {
    switch (command.type) {
        case CommandType::Move: {
            if (command.indices.size() != 2) {
                state.message =
                    "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
                return;
            }
            const int src = command.indices[0];
            const int dst = command.indices[1];
            const int amount = RuleEngine::legalPourAmount(state, src, dst);
            if (amount == 0) {
                state.message =
                    "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
                return;
            }
            std::string message;
            RuleEngine::applyMove(state, Move{src, dst, amount, 0, ""}, true, message);
            state.message.clear();
            return;
        }
        case CommandType::Restart:
            state = rebuildCurrentLevel(state);
            return;
        case CommandType::Help:
            renderer_.renderHelp();
            renderer_.waitForEnter("Press Enter to return to menu...");
            state.message.clear();
            return;
        case CommandType::Inventory:
            renderer_.renderInventory(state);
            renderer_.waitForEnter();
            state.message.clear();
            return;
        case CommandType::Panel:
            detailsPanelExpanded_ = !detailsPanelExpanded_;
            state.message = detailsPanelExpanded_ ? "Extra details shown." : "Extra details hidden.";
            return;
        case CommandType::Use:
            if (!applyPowerUp(state, command.subject, command.indices)) {
                if (state.message.empty()) {
                    state.message =
                        "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
                }
            }
            return;
        case CommandType::Save: {
            std::string error;
            if (saveManager_.saveGame(state, error)) {
                state.message = "Game saved.";
            } else {
                state.message = error;
            }
            return;
        }
        case CommandType::Hint: {
            const std::optional<Move> suggestion = RuleEngine::suggestMove(state);
            state.message = suggestion.has_value()
                                ? suggestion->explain
                                : "No legal move is available from this position.";
            return;
        }
        case CommandType::Menu:
            returnToMenu = true;
            return;
        case CommandType::Quit:
            running_ = false;
            return;
        case CommandType::Invalid:
            state.message =
                "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
            return;
    }
}

void GameController::advanceAfterLevelClear(GameState& state) {
    if (state.level >= state.totalLevels) {
        finishRun(state);
        state.level = state.totalLevels + 1;
        return;
    }

    renderer_.renderLevelClear(state);
    renderer_.waitForEnter("Press Enter to draw a lucky reward...");

    std::mt19937 rng(state.currentLevelSeed ^ static_cast<unsigned int>(state.totalMoves + 37));
    const std::string rewardName = powerUps_.randomName(rng);
    grantReward(state.inventory, rewardName);
    state.powerUpRewards[rewardName] += 1;
    stats_.powerUpRewards[rewardName] += 1;
    persistStats();
    renderer_.renderReward(rewardName);
    renderer_.waitForEnter("");

    GameState next = LevelGenerator::buildLevel(state.level + 1, state.difficulty, state.baseSeed,
                                                state.totalLevels);
    next.inventory = state.inventory;
    next.totalMoves = state.totalMoves;
    next.powerUpsUsed = state.powerUpsUsed;
    next.powerUpRewards = state.powerUpRewards;
    next.message.clear();
    state = next;
}

void GameController::finishRun(GameState& state) {
    stats_.gamesCleared += 1;
    stats_.totalMovesAcrossWins += state.totalMoves;

    const auto bestIt = stats_.bestMoves.find(state.difficulty);
    if (bestIt == stats_.bestMoves.end() || state.totalMoves < bestIt->second) {
        stats_.bestMoves[state.difficulty] = state.totalMoves;
    }
    persistStats();

    std::string ignored;
    saveManager_.deleteSave(ignored);
    while (running_) {
        renderer_.renderGameClear(state);
        std::string input;
        if (!std::getline(std::cin, input)) {
            return;
        }
        const std::string choice = toLowerCopy(trimCopy(input));
        if (choice == "r" || choice == "restart") {
            return;
        }
        if (choice == "q" || choice == "quit") {
            running_ = false;
            return;
        }
    }
}

bool GameController::applyPowerUp(GameState& state, const std::string& name,
                                  const std::vector<int>& args) {
    const PowerUp* powerUp = powerUps_.get(name);
    if (powerUp == nullptr) {
        state.message =
            "Invalid input! If you need any guidance, please type 'help' to check the valid input.";
        return false;
    }

    int* counter = inventoryCounter(state.inventory, name);
    if (counter == nullptr || *counter <= 0) {
        state.message = "You don't have this inventory yet, try another input!";
        return false;
    }

    std::string message;
    if (!powerUp->apply(state, args, message)) {
        state.message = message;
        return false;
    }

    *counter -= 1;
    state.powerUpsUsed[name] += 1;
    stats_.powerUpUses[name] += 1;
    persistStats();

    state.message = message + remainingPowerUpText(name, *counter);
    return true;
}

void GameController::persistStats() {
    std::string ignored;
    saveManager_.saveStats(stats_, ignored);
}

GameState GameController::rebuildCurrentLevel(const GameState& state) const {
    GameState rebuilt =
        LevelGenerator::buildLevel(state.level, state.difficulty, state.baseSeed, state.totalLevels);
    rebuilt.inventory = state.inventory;
    rebuilt.totalMoves = state.totalMoves;
    rebuilt.powerUpsUsed = state.powerUpsUsed;
    rebuilt.powerUpRewards = state.powerUpRewards;
    rebuilt.message.clear();
    return rebuilt;
}
