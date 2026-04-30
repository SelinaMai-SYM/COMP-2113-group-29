#include "core/GameController.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "core/LevelGenerator.h"
#include "core/Mechanics.h"
#include "core/RuleEngine.h"

namespace {

/*
- What it does:
  Converts a string to lowercase.
- Inputs:
  One text string.
- Outputs:
  The lowered copy.
*/
std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

/*
- What it does:
  Trims whitespace from both ends of a string.
- Inputs:
  One text string.
- Outputs:
  The trimmed copy.
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

/*
- What it does:
  Checks whether one input line is only an ANSI escape sequence such as arrow-key input.
- Inputs:
  One raw or trimmed input line.
- Outputs:
  True if the line starts with an escape character.
*/
bool isEscapeOnlyInput(const std::string& text) {
    return !text.empty() && static_cast<unsigned char>(text.front()) == 27U;
}

/*
- What it does:
  Discards any immediately available bytes after an escape character.
- Inputs:
  None.
- Outputs:
  Terminal escape-sequence tail bytes are consumed.
*/
void discardPendingEscapeBytes() {
    int available = 0;
    if (ioctl(STDIN_FILENO, FIONREAD, &available) != 0) {
        return;
    }
    while (available > 0) {
        char ignored = '\0';
        if (read(STDIN_FILENO, &ignored, 1) <= 0) {
            break;
        }
        if (ioctl(STDIN_FILENO, FIONREAD, &available) != 0) {
            break;
        }
    }
}

/*
- What it does:
  Checks whether the game should use the legacy raw-input prompt mode.
- Inputs:
  None.
- Outputs:
  True when the legacy redraw mode is explicitly enabled.
*/
bool useLegacyRawInputMode() {
    const char* value = std::getenv("COLORFUL_TUBE_ALT_SCREEN");
    return value != nullptr && value[0] != '\0';
}

/*
- What it does:
  Reads one command line, using plain scrollback-friendly input by default.
- Inputs:
  One mutable output string.
- Outputs:
  True on success, or false on EOF/input failure.
*/
bool readCommandLine(std::string& line) {
    line.clear();
    if (!isatty(STDIN_FILENO) || !useLegacyRawInputMode()) {
        return static_cast<bool>(std::getline(std::cin, line));
    }

    termios original{};
    if (tcgetattr(STDIN_FILENO, &original) != 0) {
        return static_cast<bool>(std::getline(std::cin, line));
    }

    termios raw = original;
    raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return static_cast<bool>(std::getline(std::cin, line));
    }

    struct RestoreTerminalMode {
        termios originalMode{};
        ~RestoreTerminalMode() {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalMode);
        }
    } restore{original};

    while (true) {
        char ch = '\0';
        const ssize_t count = read(STDIN_FILENO, &ch, 1);
        if (count <= 0) {
            return false;
        }

        if (ch == '\n' || ch == '\r') {
            std::cout << "\n" << std::flush;
            return true;
        }
        if (ch == 4 && line.empty()) {
            return false;
        }
        if (ch == 27) {
            discardPendingEscapeBytes();
            continue;
        }
        if (ch == 127 || ch == '\b') {
            if (!line.empty()) {
                line.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }

        const unsigned char visible = static_cast<unsigned char>(ch);
        if (std::isprint(visible) != 0) {
            line.push_back(ch);
            std::cout << ch << std::flush;
        }
    }
}

/*
- What it does:
  Returns a mutable inventory counter by power-up name.
- Inputs:
  The current inventory and power-up name.
- Outputs:
  A pointer to the matching counter, or nullptr if the name is unknown.
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

/*
- What it does:
  Increments the rewarded inventory item.
- Inputs:
  The current inventory and the rewarded power-up name.
- Outputs:
  None.
*/
void grantReward(Inventory& inventory, const std::string& name) {
    if (int* counter = inventoryCounter(inventory, name)) {
        *counter += 1;
    }
}

/*
- What it does:
  Builds a short success suffix showing the remaining count of one power-up.
- Inputs:
  The power-up name and the remaining amount.
- Outputs:
  A human-readable suffix string.
*/
std::string remainingPowerUpText(const std::string& name, int remaining) {
    return " Remaining " + name + ": " + std::to_string(remaining) + ".";
}

/*
- What it does:
  Ensures that every tracked power-up key exists in a string-to-count map.
- Inputs:
  One mutable power-up counter map.
- Outputs:
  Missing keys are inserted with zero values.
*/
void ensureTrackedCounts(std::map<std::string, int>& counts) {
    for (const std::string& name : powerUpNames()) {
        counts.try_emplace(name, 0);
    }
}

/*
- What it does:
  Resets per-run power-up tracking maps on a fresh game.
- Inputs:
  One mutable game state.
- Outputs:
  All per-run usage and reward counters become zero.
*/
void resetRunTracking(GameState& state) {
    for (const std::string& name : powerUpNames()) {
        state.powerUpsUsed[name] = 0;
        state.powerUpRewards[name] = 0;
    }
}

/*
- What it does:
  Ensures that loaded or rebuilt states contain all run-tracking counters.
- Inputs:
  One mutable game state.
- Outputs:
  Missing run-tracking keys are inserted with zero values.
*/
void ensureRunTracking(GameState& state) {
    ensureTrackedCounts(state.powerUpsUsed);
    ensureTrackedCounts(state.powerUpRewards);
}

struct GameScreenGuard {
    const Renderer& renderer;

    /*
    - What it does:
      Leaves the dedicated gameplay screen automatically when the guard expires.
    - Inputs:
      None.
    - Outputs:
      The renderer restores the normal terminal screen state.
    */
    ~GameScreenGuard() {
        renderer.leaveGameScreen();
    }
};

}  // namespace

/*
- What it does:
  Constructs the controller and loads persistent statistics into memory.
- Inputs:
  None.
- Outputs:
  A ready-to-run game controller.
*/
GameController::GameController() : stats_(saveManager_.loadStats()) {
    ensureTrackedCounts(stats_.powerUpUses);
    ensureTrackedCounts(stats_.powerUpRewards);
}

void GameController::run() {
    renderer_.renderIntro();
    renderer_.waitForEnter("Press Enter to open the main menu...");

    while (running_) {
        const std::vector<SaveSlotInfo> availableSaves = saveManager_.listSaves();
        renderer_.renderMenu(stats_, availableSaves.size());

        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            break;
        }

        const std::string trimmed = trimCopy(input);
        if (isEscapeOnlyInput(trimmed)) {
            continue;
        }

        const std::string choice = toLowerCopy(trimmed);
        if (choice == "s" || choice == "start") {
            Difficulty difficulty = Difficulty::Normal;
            if (chooseDifficulty(difficulty)) {
                startNewGame(difficulty);
            }
        } else if (choice == "load" || choice == "l") {
            loadSavedGame(availableSaves);
        } else if (choice == "h" || choice == "help") {
            renderer_.renderHelp();
            renderer_.waitForEnter("Press Enter to return to menu...");
        } else if (choice == "stats" || choice == "t") {
            renderer_.renderStats(stats_);
            renderer_.waitForEnter("Press Enter to return to menu...");
        } else if (choice == "q" || choice == "quit") {
            running_ = false;
        } else {
            std::cout << "Invalid option. Enter S/L/T/H/Q\n";
            renderer_.waitForEnter();
        }
    }

    renderer_.leaveGameScreen();
    std::cout << "\nThanks for playing!\n";
}

void GameController::startNewGame(Difficulty difficulty) {
    GameState state = LevelGenerator::buildLevel(1, difficulty, SaveManager::defaultSeed());
    resetRunTracking(state);
    syncMechanicState(state);
    activeSavePath_.clear();

    stats_.gamesStarted += 1;
    persistStats();
    playGame(state);
}

void GameController::loadSavedGame(const std::vector<SaveSlotInfo>& availableSaves) {
    if (availableSaves.empty()) {
        std::cout << "\nNo saved runs were found.\n";
        renderer_.waitForEnter("Press Enter to return to menu...");
        return;
    }

    std::vector<SaveSlotInfo> currentSaves = availableSaves;
    std::string statusMessage;
    while (running_) {
        if (currentSaves.empty()) {
            std::cout << "\nNo saved runs remain.\n";
            renderer_.waitForEnter("Press Enter to return to menu...");
            return;
        }

        const SaveListAction action = chooseSaveAction(currentSaves, statusMessage);
        statusMessage.clear();
        if (!running_ || action.type == SaveListActionType::Back) {
            return;
        }

        const SaveSlotInfo selected = currentSaves[action.index];
        if (action.type == SaveListActionType::Load) {
            std::string error;
            std::optional<GameState> state = saveManager_.loadGame(selected.path, error);
            if (!state.has_value()) {
                std::cout << "\n" << error << "\n";
                renderer_.waitForEnter("Press Enter to return to menu...");
                return;
            }
            ensureRunTracking(*state);
            syncMechanicState(*state);
            activeSavePath_ = selected.path;
            playGame(*state);
            return;
        }

        if (action.type == SaveListActionType::Delete) {
            if (!confirmDelete(selected)) {
                if (!running_) {
                    return;
                }
                statusMessage = "Deletion cancelled.";
                continue;
            }

            std::string error;
            if (!saveManager_.deleteSave(selected.path, error)) {
                statusMessage = error;
                continue;
            }
            if (activeSavePath_ == selected.path) {
                activeSavePath_.clear();
            }
            currentSaves = saveManager_.listSaves();
            statusMessage = "Deleted \"" + selected.displayName + "\".";
            continue;
        }

        if (action.type == SaveListActionType::Rename) {
            statusMessage = renameSaveSlot(selected);
            if (!running_) {
                return;
            }
            currentSaves = saveManager_.listSaves();
        }
    }
}

void GameController::playGame(GameState state) {
    GameScreenGuard gameScreenGuard{renderer_};
    detailsPanelExpanded_ = false;
    bool returnToMenu = false;
    bool rebuiltSolvedOpeningOnce = false;
    while (running_ && !returnToMenu) {
        if (state.moves == 0 && RuleEngine::isSolved(state)) {
            if (rebuiltSolvedOpeningOnce) {
                renderer_.waitForEnter(
                    "Level generation failed twice and produced an already-cleared board. Press Enter to return to menu...");
                return;
            }
            rebuiltSolvedOpeningOnce = true;
            state = rebuildCurrentLevel(state);
            state.message = "Generation anomaly detected. This level was rebuilt automatically.";
            continue;
        }
        rebuiltSolvedOpeningOnce = false;

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

        std::string deadlockMessage;
        if (RuleEngine::resolveDeadlock(state, deadlockMessage)) {
            state.message = deadlockMessage;
        }

        renderer_.renderGame(state, saveManager_.hasSave(), detailsPanelExpanded_);
        std::string line;
        if (!readCommandLine(line)) {
            running_ = false;
            return;
        }

        if (isEscapeOnlyInput(trimCopy(line))) {
            state.message.clear();
            continue;
        }

        handleGameCommand(state, parser_.parse(line), returnToMenu);
    }
}

bool GameController::chooseDifficulty(Difficulty& difficulty) {
    while (running_) {
        renderer_.renderDifficultyMenu();
        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            return false;
        }

        const std::string trimmed = trimCopy(input);
        if (isEscapeOnlyInput(trimmed)) {
            continue;
        }

        const std::string choice = toLowerCopy(trimmed);
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
            state.message =
                detailsPanelExpanded_ ? "Right-side run sidebar shown."
                                      : "Right-side run sidebar hidden.";
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
            saveCurrentGame(state);
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
    if (state.mechanics.rewardBlocked) {
        renderer_.waitForEnter("Press Enter to continue. Deadlock relief forfeited this level's reward...");
    } else {
        renderer_.waitForEnter("Press Enter to draw a lucky reward...");

        std::mt19937 rng(state.currentLevelSeed ^ static_cast<unsigned int>(state.totalMoves + 37));
        const std::string rewardName = powerUps_.randomName(rng);
        grantReward(state.inventory, rewardName);
        state.powerUpRewards[rewardName] += 1;
        stats_.powerUpRewards[rewardName] += 1;
        persistStats();
        renderer_.renderReward(rewardName);
        renderer_.waitForEnter("");
    }

    GameState next = LevelGenerator::buildLevel(state.level + 1, state.difficulty, state.baseSeed,
                                                state.totalLevels);
    next.inventory = state.inventory;
    next.totalMoves = state.totalMoves;
    next.powerUpsUsed = state.powerUpsUsed;
    next.powerUpRewards = state.powerUpRewards;
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
    saveManager_.deleteSave(activeSavePath_, ignored);
    activeSavePath_.clear();
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

bool GameController::saveCurrentGame(GameState& state) {
    while (running_) {
        std::string saveName;
        if (!promptForSaveName(saveName)) {
            if (running_) {
                state.message = "Save cancelled.";
            }
            return false;
        }

        if (const std::optional<SaveSlotInfo> existing = saveManager_.findSaveByName(saveName);
            existing.has_value()) {
            const std::optional<bool> overwrite = confirmOverwrite(*existing);
            if (!overwrite.has_value()) {
                state.message = "Save cancelled.";
                return false;
            }
            if (!overwrite.value()) {
                continue;
            }
        }

        std::string savedPath;
        std::string error;
        if (!saveManager_.saveGame(state, saveName, savedPath, error)) {
            state.message = error;
            return false;
        }

        activeSavePath_ = savedPath;
        state.message = "Game saved as \"" + saveName + "\".";
        return true;
    }
    return false;
}

bool GameController::promptForSaveName(std::string& saveName) {
    while (running_) {
        std::cout << "\nEnter a save name (letters, numbers, spaces, '-' and '_' only).\n";
        std::cout << "Use a new name to create another save, or reuse one to overwrite it.\n";
        std::cout << "Leave it blank to cancel.\n> " << std::flush;

        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            return false;
        }

        if (trimCopy(input).empty()) {
            return false;
        }

        std::string error;
        if (!SaveManager::isValidSaveName(input, saveName, error)) {
            std::cout << "\n" << error << "\n";
            continue;
        }
        return true;
    }
    return false;
}

std::optional<bool> GameController::confirmOverwrite(const SaveSlotInfo& existingSave) {
    while (running_) {
        std::cout << "\nA save named \"" << existingSave.displayName << "\" already exists.\n";
        std::cout << "Saved: " << existingSave.savedAtLabel << " | "
                  << difficultyLabel(existingSave.difficulty) << " | Level "
                  << existingSave.level << "/" << existingSave.totalLevels;
        if (existingSave.legacySlot) {
            std::cout << " | legacy";
        }
        std::cout << "\nEnter O to overwrite, R to rename, or C to cancel.\n> " << std::flush;

        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            return std::nullopt;
        }

        const std::string choice = toLowerCopy(trimCopy(input));
        if (choice == "o" || choice == "overwrite") {
            return true;
        }
        if (choice == "r" || choice == "rename") {
            return false;
        }
        if (choice.empty() || choice == "c" || choice == "cancel") {
            return std::nullopt;
        }
        std::cout << "\nInvalid choice. Enter O, R, or C.\n";
    }
    return std::nullopt;
}

bool GameController::confirmDelete(const SaveSlotInfo& selectedSave) {
    while (running_) {
        std::cout << "\nDelete \"" << selectedSave.displayName << "\"?\n";
        std::cout << "Saved: " << selectedSave.savedAtLabel << " | "
                  << difficultyLabel(selectedSave.difficulty) << " | Level "
                  << selectedSave.level << "/" << selectedSave.totalLevels;
        if (selectedSave.legacySlot) {
            std::cout << " | legacy";
        }
        std::cout << "\nEnter Y to delete or C to cancel.\n> " << std::flush;

        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            return false;
        }

        const std::string choice = toLowerCopy(trimCopy(input));
        if (choice == "y" || choice == "yes" || choice == "delete") {
            return true;
        }
        if (choice.empty() || choice == "c" || choice == "cancel" || choice == "n" ||
            choice == "no") {
            return false;
        }

        std::cout << "\nInvalid choice. Enter Y or C.\n";
    }
    return false;
}

std::string GameController::renameSaveSlot(const SaveSlotInfo& selectedSave) {
    while (running_) {
        std::cout << "\nRenaming \"" << selectedSave.displayName << "\".\n";

        std::string newName;
        if (!promptForSaveName(newName)) {
            return running_ ? "Rename cancelled." : "";
        }

        if (const std::optional<SaveSlotInfo> existing = saveManager_.findSaveByName(newName);
            existing.has_value() && existing->path != selectedSave.path) {
            const std::optional<bool> overwrite = confirmOverwrite(*existing);
            if (!overwrite.has_value()) {
                return running_ ? "Rename cancelled." : "";
            }
            if (!overwrite.value()) {
                continue;
            }
        }

        std::string renamedPath;
        std::string error;
        if (!saveManager_.renameSave(selectedSave.path, newName, renamedPath, error)) {
            return error;
        }

        if (activeSavePath_ == selectedSave.path) {
            activeSavePath_ = renamedPath;
        }
        return "Renamed \"" + selectedSave.displayName + "\" to \"" + newName + "\".";
    }
    return "";
}

GameController::SaveListAction GameController::chooseSaveAction(
    const std::vector<SaveSlotInfo>& saves, const std::string& statusMessage) {
    std::string message = statusMessage;
    while (running_) {
        renderer_.renderSaveList(saves, message);
        message.clear();

        std::string input;
        if (!readCommandLine(input)) {
            running_ = false;
            return SaveListAction{};
        }

        const std::string trimmed = trimCopy(input);
        const std::string lowered = toLowerCopy(trimmed);
        if (lowered == "b" || lowered == "back" || lowered == "m" || lowered == "menu") {
            return SaveListAction{SaveListActionType::Back, 0};
        }

        std::istringstream stream(trimmed);
        int selection = 0;
        if ((stream >> selection) && stream.eof() && selection >= 1 &&
            selection <= static_cast<int>(saves.size())) {
            return SaveListAction{SaveListActionType::Load,
                                  static_cast<std::size_t>(selection - 1)};
        }

        std::istringstream actionStream(lowered);
        std::string command;
        int commandIndex = 0;
        if ((actionStream >> command >> commandIndex) && actionStream.eof() && commandIndex >= 1 &&
            commandIndex <= static_cast<int>(saves.size())) {
            if (command == "d" || command == "delete") {
                return SaveListAction{SaveListActionType::Delete,
                                      static_cast<std::size_t>(commandIndex - 1)};
            }
            if (command == "r" || command == "rename") {
                return SaveListAction{SaveListActionType::Rename,
                                      static_cast<std::size_t>(commandIndex - 1)};
            }
        }

        message =
            "Invalid save selection. Here N means the save number shown at the start of each row.";
    }
    return SaveListAction{};
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
    syncMechanicState(rebuilt);
    return rebuilt;
}
