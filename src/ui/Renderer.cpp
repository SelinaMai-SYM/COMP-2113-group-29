#include "ui/Renderer.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <vector>

#include "ui/Ansi.h"

namespace {

const std::string kAccent = "magenta";
const std::string kFrame = "yellow";
const std::string kText = "white";
const std::string kTitle = "magenta";
const std::string kTopLeft = "┌";
const std::string kTopRight = "┐";
const std::string kBottomLeft = "└";
const std::string kBottomRight = "┘";
const std::string kHorizontal = "─";
const std::string kVertical = "│";
const std::size_t kMenuWidth = 70;
const std::size_t kMinBoardWidth = 34;
const std::size_t kMinSidebarWidth = 38;
const std::size_t kMinPageWidth = 78;
const std::size_t kPreferredBoxInnerWidth = 94;
const std::size_t kColumnGapWidth = 2;

/**
 * What it does:
 * Returns the longest visible line length in one panel.
 * Inputs:
 * One panel line buffer.
 * Outputs:
 * The maximum ANSI-free visible width.
 */
std::size_t longestVisibleLine(const std::vector<std::string>& lines) {
    std::size_t longest = 0;
    for (const std::string& line : lines) {
        longest = std::max(longest, ansi::visibleLength(line));
    }
    return longest;
}

/**
 * What it does:
 * Repeats one text fragment several times.
 * Inputs:
 * The fragment and the repetition count.
 * Outputs:
 * A concatenated string.
 */
std::string repeatText(const std::string& text, std::size_t count) {
    std::string out;
    for (std::size_t index = 0; index < count; ++index) {
        out += text;
    }
    return out;
}

/**
 * What it does:
 * Reads one named counter from a string-to-int map without mutating it.
 * Inputs:
 * The source map and desired key.
 * Outputs:
 * The stored count or zero when the key is absent.
 */
int getTrackedCount(const std::map<std::string, int>& counts, const std::string& key) {
    const auto it = counts.find(key);
    return (it == counts.end()) ? 0 : it->second;
}

/**
 * What it does:
 * Splits one plain-text sentence into wrapped lines.
 * Inputs:
 * The source text and preferred maximum width.
 * Outputs:
 * A vector of wrapped plain-text lines.
 */
std::vector<std::string> wrapText(const std::string& text, std::size_t width) {
    if (text.empty() || text.size() <= width) {
        return {text};
    }

    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string current;
    while (stream >> word) {
        if (current.empty()) {
            current = word;
            continue;
        }
        if (current.size() + 1 + word.size() <= width) {
            current += " " + word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back(text);
    }
    return lines;
}

/**
 * What it does:
 * Adds one titled section to a panel.
 * Inputs:
 * The destination lines, section title and section body.
 * Outputs:
 * The buffer gains one spaced block.
 */
void appendSection(std::vector<std::string>& lines, const std::string& title,
                   const std::vector<std::string>& body) {
    lines.push_back(ansi::style(title, kAccent, "", true, true));
    lines.insert(lines.end(), body.begin(), body.end());
    lines.push_back("");
}

/**
 * What it does:
 * Converts a map of power-up counters into printable lines.
 * Inputs:
 * One string-to-int counter map.
 * Outputs:
 * A vector of four formatted lines.
 */
std::vector<std::string> powerUpCountLines(const std::map<std::string, int>& counts) {
    std::vector<std::string> lines;
    for (const std::string& name : powerUpNames()) {
        lines.push_back(name + ": " + std::to_string(getTrackedCount(counts, name)));
    }
    return lines;
}

/**
 * What it does:
 * Counts how many tubes are complete and uniformly filled.
 * Inputs:
 * The current game state.
 * Outputs:
 * The number of complete solved tubes.
 */
int countCompleteTubes(const GameState& state) {
    int complete = 0;
    for (const Tube& tube : state.tubes) {
        if (tube.isUniformFull()) {
            ++complete;
        }
    }
    return complete;
}

/**
 * What it does:
 * Counts how many tubes are empty.
 * Inputs:
 * The current game state.
 * Outputs:
 * The number of empty tubes.
 */
int countEmptyTubes(const GameState& state) {
    int empty = 0;
    for (const Tube& tube : state.tubes) {
        if (tube.isEmpty()) {
            ++empty;
        }
    }
    return empty;
}

/**
 * What it does:
 * Converts the tube state into a top-down grid for rendering.
 * Inputs:
 * The current game state.
 * Outputs:
 * A matrix of block identifiers.
 */
std::vector<std::vector<char>> buildGrid(const GameState& state) {
    std::vector<std::vector<char>> grid(state.capacity,
                                        std::vector<char>(state.tubes.size(), '.'));
    for (std::size_t column = 0; column < state.tubes.size(); ++column) {
        const std::vector<char>& slots = state.tubes[column].slots();
        for (std::size_t index = 0; index < slots.size(); ++index) {
            const int row = state.capacity - 1 - static_cast<int>(index);
            grid[row][column] = slots[index];
        }
    }
    return grid;
}

/**
 * What it does:
 * Builds the left board panel containing title, tubes and tube numbers.
 * Inputs:
 * The current game state plus a custom title and subtitle.
 * Outputs:
 * A vector of printable strings.
 */
std::vector<std::string> buildBoardLines(const GameState& state, const std::string& title,
                                         const std::string& subtitle = "") {
    std::vector<std::string> lines;
    lines.push_back(ansi::style(title, kTitle, "", true, true));
    if (!subtitle.empty()) {
        lines.push_back(subtitle);
    }
    lines.push_back("");

    const std::vector<std::vector<char>> grid = buildGrid(state);
    for (int row = 0; row < state.capacity; ++row) {
        std::ostringstream out;
        for (std::size_t column = 0; column < state.tubes.size(); ++column) {
            if (column > 0) {
                out << " ";
            }
            out << ansi::cell(grid[row][column], 2);
        }
        lines.push_back(out.str());
    }

    std::ostringstream numbers;
    for (std::size_t column = 0; column < state.tubes.size(); ++column) {
        if (column > 0) {
            numbers << " ";
        }
        numbers << ansi::center(std::to_string(column + 1), 4);
    }
    lines.push_back(numbers.str());
    return lines;
}

/**
 * What it does:
 * Builds the in-game sidebar with stats, objectives and optional run details.
 * Inputs:
 * The current state, save-file availability and detail-toggle state.
 * Outputs:
 * A vector of printable sidebar lines.
 */
std::vector<std::string> buildSidebarLines(const GameState& state, bool hasSave, bool showDetails) {
    std::vector<std::string> lines;
    (void)hasSave;
    lines.push_back(ansi::style("STATS | Level: " + std::to_string(state.level) + "/" +
                                    std::to_string(state.totalLevels),
                                kAccent, "", true, true));
    lines.push_back("Moves (this level): " + std::to_string(state.moves));
    lines.push_back("Total moves: " + std::to_string(state.totalMoves));
    lines.push_back("Complete tubes: " + std::to_string(countCompleteTubes(state)));
    lines.push_back("Empty tubes: " + std::to_string(countEmptyTubes(state)));
    lines.push_back("");
    lines.push_back(ansi::style("INVENTORY", kAccent, "", true, true));
    lines.push_back("bin: " + std::to_string(state.inventory.bin) +
                    " | undo: " + std::to_string(state.inventory.undo) +
                    " | straw: " + std::to_string(state.inventory.straw) +
                    " | reverse: " + std::to_string(state.inventory.reverse));
    lines.push_back("");
    lines.push_back(ansi::style("OBJECTIVES", kAccent, "", true, true));
    lines.push_back("- Make each tube empty or fully filled with one color");
    lines.push_back("- Pour blocks to any tube with space.");
    lines.push_back("- Type two numbers, e.g., 1 6.");

    if (showDetails) {
        lines.push_back("");
        lines.push_back(ansi::style("EXTRA", kAccent, "", true, true));
        lines.push_back("Mode: " + difficultyLabel(state.difficulty));
        lines.push_back("Seed: " + std::to_string(state.baseSeed));
        lines.push_back("Level seed: " + std::to_string(state.currentLevelSeed));
    }

    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/**
 * What it does:
 * Builds the sidebar shown on the level-clear screen.
 * Inputs:
 * The solved level state.
 * Outputs:
 * A vector of printable summary lines.
 */
std::vector<std::string> buildLevelClearSidebar(const GameState& state) {
    std::vector<std::string> lines;
    lines.push_back(ansi::style("LEVEL " + std::to_string(state.level) + " CLEARED!", kAccent, "",
                                true, true));
    lines.push_back("");
    lines.push_back("Moves (this level): " + std::to_string(state.moves));
    lines.push_back("Total moves: " + std::to_string(state.totalMoves));
    lines.push_back("Complete tubes: " + std::to_string(countCompleteTubes(state)));
    lines.push_back("Empty tubes: " + std::to_string(countEmptyTubes(state)));
    lines.push_back("");
    lines.push_back(ansi::style("INVENTORY", kAccent, "", true, true));
    lines.push_back("bin: " + std::to_string(state.inventory.bin) +
                    " | undo: " + std::to_string(state.inventory.undo) +
                    " | straw: " + std::to_string(state.inventory.straw) +
                    " | reverse: " + std::to_string(state.inventory.reverse));
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/**
 * What it does:
 * Builds the sidebar shown on the final game-clear screen.
 * Inputs:
 * The final solved game state.
 * Outputs:
 * A vector of printable run-summary lines.
 */
std::vector<std::string> buildGameClearSidebar(const GameState& state) {
    std::vector<std::string> lines;
    lines.push_back("Total moves: " + std::to_string(state.totalMoves));
    lines.push_back("");
    lines.push_back(ansi::style("Power-ups used:", kText, "", true));
    for (const auto& name : powerUpNames()) {
        lines.push_back(" " + name + ": " + std::to_string(getTrackedCount(state.powerUpsUsed, name)));
    }
    lines.push_back("");
    lines.push_back(ansi::style("R(estart) Q(uit)", kText, "", true));
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/**
 * What it does:
 * Prints one styled two-column box with flexible widths.
 * Inputs:
 * Left and right line buffers.
 * Outputs:
 * A Unicode-framed layout on standard output.
 */
void renderTwoColumnBox(const std::vector<std::string>& left, const std::vector<std::string>& right) {
    std::size_t leftWidth = std::max(kMinBoardWidth, longestVisibleLine(left));
    std::size_t rightWidth = std::max(kMinSidebarWidth, longestVisibleLine(right));
    const std::size_t requiredInnerWidth = leftWidth + rightWidth + kColumnGapWidth;
    const int detectedWidth = ansi::terminalWidth();
    const std::size_t availableInnerWidth =
        detectedWidth > 2 ? static_cast<std::size_t>(detectedWidth - 2) : requiredInnerWidth;
    const std::size_t targetInnerWidth =
        std::max(requiredInnerWidth, std::min(kPreferredBoxInnerWidth, availableInnerWidth));

    if (targetInnerWidth > requiredInnerWidth) {
        const std::size_t extraWidth = targetInnerWidth - requiredInnerWidth;
        const std::size_t leftExtra = (extraWidth * 3) / 5;
        leftWidth += leftExtra;
        rightWidth += extraWidth - leftExtra;
    }

    const std::size_t innerWidth = leftWidth + rightWidth + kColumnGapWidth;
    std::cout << ansi::style(kTopLeft + repeatText(kHorizontal, innerWidth) + kTopRight, kFrame)
              << "\n";

    const std::size_t rowCount = std::max(left.size(), right.size());
    for (std::size_t index = 0; index < rowCount; ++index) {
        const std::string leftText = (index < left.size()) ? left[index] : "";
        const std::string rightText = (index < right.size()) ? right[index] : "";
        std::cout << ansi::style(kVertical, kFrame) << ansi::padRight(leftText, leftWidth) << " "
                  << ansi::style(kVertical, kFrame) << ansi::padRight(rightText, rightWidth)
                  << ansi::style(kVertical, kFrame) << "\n";
    }

    std::cout << ansi::style(kBottomLeft + repeatText(kHorizontal, innerWidth) + kBottomRight, kFrame)
              << "\n";
}

/**
 * What it does:
 * Prints one styled single-column box.
 * Inputs:
 * One line buffer and a preferred minimum width.
 * Outputs:
 * A framed single-column layout on standard output.
 */
void renderSingleColumnBox(const std::vector<std::string>& lines, std::size_t minWidth) {
    const std::size_t innerWidth = std::max(minWidth, longestVisibleLine(lines));
    std::cout << ansi::style(kTopLeft + repeatText(kHorizontal, innerWidth) + kTopRight, kFrame)
              << "\n";
    for (const std::string& line : lines) {
        std::cout << ansi::style(kVertical, kFrame) << ansi::padRight(line, innerWidth)
                  << ansi::style(kVertical, kFrame) << "\n";
    }
    std::cout << ansi::style(kBottomLeft + repeatText(kHorizontal, innerWidth) + kBottomRight, kFrame)
              << "\n";
}

/**
 * What it does:
 * Prints an animated title used on the menu page.
 * Inputs:
 * None.
 * Outputs:
 * A title sequence that lasts about two seconds on interactive terminals.
 */
void animateMenuTitle() {
    const std::string title = "COLOR SORT PUZZLE GAME";
    if (!ansi::supportsColor()) {
        std::cout << ansi::center(title, kMenuWidth) << "\n";
        return;
    }

    std::string current;
    const int totalDurationMs = 2000;
    const int perCharMs =
        std::max(20, totalDurationMs / static_cast<int>(std::max<std::size_t>(1, title.size())));
    for (char ch : title) {
        current.push_back(ch);
        std::cout << "\r"
                  << ansi::style(ansi::center(current, kMenuWidth), kAccent, "", true, true)
                  << std::flush;
        ansi::sleepMs(perCharMs);
    }
    std::cout << "\n";
}

/**
 * What it does:
 * Renders a short command reminder below the gameplay box.
 * Inputs:
 * None.
 * Outputs:
 * A compact three-line command guide.
 */
void renderGameGuide() {
    std::cout << ansi::style("COMMANDS:", kAccent, "", true, true) << "\n";
    std::cout << " Move:       e.g. '1 6' - pour from tube 1 to tube 6\n";
    std::cout << " Restart:    'r' or 'restart' - restart current level\n";
    std::cout << " Inventory:  'i' or 'inv' - view your power-ups\n";
    std::cout << " Use power-up: 'use bin 3' / 'use undo' / 'use straw 2 3' / 'use reverse 4'\n";
    std::cout << " Help:       'help' - view detailed help\n";
    std::cout << " Quit:       'q' or 'quit' - exit game\n";
}

/**
 * What it does:
 * Builds and prints a small confetti block.
 * Inputs:
 * A random generator and whether repeated animation frames should be shown.
 * Outputs:
 * Five rows of celebratory characters.
 */
void renderConfetti(std::mt19937& rng, bool animate) {
    const std::string confettiChars = "*+.: ";
    const std::vector<std::string> colors = {"red", "green", "yellow",
                                             "blue", "magenta", "cyan", "white"};

    auto printFrame = [&]() {
        for (int row = 0; row < 5; ++row) {
            std::string line;
            for (int column = 0; column < 60; ++column) {
                line.push_back(confettiChars[rng() % confettiChars.size()]);
            }
            std::cout << ansi::style(line, colors[rng() % colors.size()]) << "\n";
        }
    };

    if (!animate) {
        printFrame();
        return;
    }

    for (int frame = 0; frame < 20; ++frame) {
        printFrame();
        ansi::sleepMs(60);
        if (frame < 19) {
            std::cout << ansi::moveCursorUp(5);
        }
    }
}

}  // namespace

void Renderer::renderMenu(const StatsRecord& stats, bool hasSave) const {
    leaveGameScreen();
    (void)stats;
    (void)hasSave;
    std::cout << ansi::clearScreen();
    std::cout << "\n" << ansi::style(ansi::repeat('=', kMenuWidth), kAccent) << "\n";
    animateMenuTitle();
    std::cout << ansi::style(ansi::repeat('=', kMenuWidth), kAccent) << "\n\n";

    std::cout << ansi::style("GAME RULES:", kAccent, "", true, true) << "\n";
    std::cout << " * Goal: Make each tube either EMPTY or FULLY FILLED with ONE COLOR\n";
    std::cout << " * Move: Pour color blocks to any tube with space\n";
    std::cout << " * Levels: 15 levels with increasing difficulty\n";
    std::cout << " * Power-ups: Earn one reward per completed level!\n\n";

    std::cout << ansi::style("AVAILABLE POWER-UPS (Lucky Draw rewards):", kAccent, "", true, true)
              << "\n";
    std::cout << " Each cleared level gives you ONE random power-up:\n\n";
    std::cout << " 1. BIN - Delete top block from a non-empty tube\n";
    std::cout << " Command: use bin 3\n\n";
    std::cout << " 2. UNDO - Undo last regular move (current level only)\n";
    std::cout << " Command: use undo\n\n";
    std::cout << " 3. STRAW - Move bottom block from one tube to top of another tube\n";
    std::cout << " Command: use straw 2 3  (move from tube 2 to tube 3)\n\n";
    std::cout << " 4. REVERSE - Reverse the order of an incomplete tube\n";
    std::cout << " Command: use reverse 4\n\n";

    std::cout << ansi::style("TIP: Power-ups persist across all levels!", kAccent, "", true)
              << "\n\n";
    std::cout << ansi::style(ansi::repeat('=', kMenuWidth), kFrame) << "\n\n";
    std::cout << ansi::style(" [S] Start Game", kText, "", true) << "\n";
    std::cout << ansi::style(" [H] Help & Controls", kText, "", true) << "\n";
    std::cout << ansi::style(" [Q] Quit", kText, "", true) << "\n\n";
    std::cout << ansi::style(ansi::repeat('=', kMenuWidth), kFrame) << "\n\n";
    std::cout << "\n> " << std::flush;
}

void Renderer::renderDifficultyMenu() const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::cout << ansi::style("SELECT MODE", kAccent, "", true, true) << "\n\n";
    std::cout << " 1. Easy   - more empty tubes, slower scaling\n";
    std::cout << " 2. Normal - default balance\n";
    std::cout << " 3. Hard   - denser puzzles and fewer empty tubes later\n";
    std::cout << " b. Back\n\n";
    std::cout << " Set SEED before launch if you want reproducible layouts.\n\n";
    std::cout << "\n> " << std::flush;
}

void Renderer::renderGame(const GameState& state, bool hasSave, bool showDetails) const {
    enterGameScreen();
    std::cout << ansi::clearScreen();
    renderTwoColumnBox(buildBoardLines(state, "Color Sort (number input)"),
                       buildSidebarLines(state, hasSave, showDetails));
    std::cout << "\n";
    renderGameGuide();
    if (!state.message.empty()) {
        std::cout << "\n";
        for (const std::string& line : wrapText(state.message, 92)) {
            std::cout << line << "\n";
        }
    }
    std::cout << "\n> " << std::flush;
}

void Renderer::renderHelp() const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::cout << "\n";
    std::cout << ansi::style("HELP & CONTROLS", kAccent, "", true, true) << "\n\n";
    std::cout << ansi::style("How to Run:", kAccent, "", true) << "\n";
    std::cout << " ./color_tube\n\n";
    std::cout << ansi::style("Basic Controls:", kAccent, "", true) << "\n";
    std::cout << " * Move: '1 6' - pour from tube 1 to tube 6\n";
    std::cout << " * Restart: 'r' or 'restart'\n";
    std::cout << " * Quit: 'q' or 'quit'\n\n";
    std::cout << ansi::style("Inventory:", kAccent, "", true) << "\n";
    std::cout << " * View: 'i' or 'inv'\n";
    std::cout << " * Use bin: 'use bin 5'\n";
    std::cout << " * Use straw: 'use straw 2 3' (move from tube 2 to tube 3)\n";
    std::cout << " * Use reverse: 'use reverse 7'\n";
    std::cout << " * Use undo: 'use undo'\n\n";
    std::cout << ansi::style("Levels:", kAccent, "", true) << "\n";
    std::cout << " * 15 levels total\n";
    std::cout << " * Depth increases every 3 levels\n";
    std::cout << " * Colors increase by 1 each level (max 8)\n";
    std::cout << " * Always 2 empty tubes\n\n";
    std::cout << ansi::style("Lucky Draw:", kAccent, "", true) << "\n";
    std::cout << " * One draw per cleared level\n";
    std::cout << " * Items persist across levels\n";
    std::cout << " * Inventory resets on new game\n\n";
    std::cout << ansi::style("SEED:", kAccent, "", true) << "\n";
    std::cout << " * Set SEED environment variable for reproducible runs\n";
    std::cout << " * Example: SEED=1330 ./color_tube\n\n";
}

void Renderer::renderInventory(const GameState& state) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::cout << "\n";
    std::cout << "INVENTORY (Available power-ups):\n";
    std::cout << " bin: " << state.inventory.bin << "\n";
    std::cout << " undo: " << state.inventory.undo << "\n";
    std::cout << " straw: " << state.inventory.straw << "\n";
    std::cout << " reverse: " << state.inventory.reverse << "\n\n";
    std::cout << "Note: Power-ups persist across all levels until you quit the game.\n\n";
}

void Renderer::renderStats(const StatsRecord& stats) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();

    std::ostringstream average;
    if (stats.gamesCleared > 0) {
        average << std::fixed << std::setprecision(1)
                << static_cast<double>(stats.totalMovesAcrossWins) / stats.gamesCleared;
    } else {
        average << "-";
    }

    std::vector<std::string> lines;
    lines.push_back(ansi::style("PERSISTENT STATS", kTitle, "", true, true));
    lines.push_back("");
    appendSection(lines, "RUN HISTORY",
                  {"Games started: " + std::to_string(stats.gamesStarted),
                   "Games cleared: " + std::to_string(stats.gamesCleared),
                   "Total winning moves: " + std::to_string(stats.totalMovesAcrossWins),
                   "Average winning moves: " + average.str()});
    appendSection(lines, "BEST CLEARS",
                  {"Easy: " +
                       (stats.bestMoves.count(Difficulty::Easy)
                            ? std::to_string(stats.bestMoves.at(Difficulty::Easy))
                            : "-"),
                   "Normal: " +
                       (stats.bestMoves.count(Difficulty::Normal)
                            ? std::to_string(stats.bestMoves.at(Difficulty::Normal))
                            : "-"),
                   "Hard: " +
                       (stats.bestMoves.count(Difficulty::Hard)
                            ? std::to_string(stats.bestMoves.at(Difficulty::Hard))
                            : "-")});
    appendSection(lines, "REWARDS DRAWN", powerUpCountLines(stats.powerUpRewards));
    appendSection(lines, "POWER-UP USES", powerUpCountLines(stats.powerUpUses));
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }

    renderSingleColumnBox(lines, kMinPageWidth);
}

void Renderer::renderLevelClear(const GameState& state) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n";
    std::cout << ansi::style(
                     ansi::center("YOU PASSED LEVEL " + std::to_string(state.level) + "!", 60),
                     kAccent, "", true, true)
              << "\n\n";

    std::mt19937 rng(state.currentLevelSeed + 13U);
    renderConfetti(rng, ansi::supportsScreenControl());
    std::cout << "\n";
    renderTwoColumnBox(buildBoardLines(state, "Color Sort - FINAL STATE"),
                       buildLevelClearSidebar(state));
    std::cout << "\n";
}

void Renderer::renderReward(const std::string& rewardName) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n";

    const std::string spinner = "|/-\\";
    const std::string label = "Drawing reward ";
    for (int step = 0; step < 40; ++step) {
        std::cout << "\r" << ansi::style(label, kText)
                  << ansi::style(std::string(1, spinner[step % spinner.size()]), kAccent, "", true)
                  << std::flush;
        ansi::sleepMs(30);
    }
    std::cout << "\r" << std::string(label.size() + 1, ' ') << "\r";
    std::string upper = rewardName;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    std::cout << ansi::style("You got: " + upper + "!", kAccent, "", true, true) << "\n\n";
    std::cout << "Press Enter to continue to next level...\n";
}

void Renderer::renderGameClear(const GameState& state) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n";
    std::cout << ansi::style("GAME CLEAR!!!", kAccent, "", true, true) << "\n\n";
    for (const std::string& line : buildGameClearSidebar(state)) {
        std::cout << line << "\n";
    }
    std::cout << "\n> " << std::flush;
}

void Renderer::waitForEnter(const std::string& prompt) const {
    std::cout << ansi::style(prompt, kAccent, "", true) << std::flush;
    std::string ignored;
    std::getline(std::cin, ignored);
}

void Renderer::enterGameScreen() const {
    if (gameScreenActive_) {
        return;
    }
    const std::string sequence = ansi::enterAlternateScreen();
    if (!sequence.empty()) {
        std::cout << sequence;
        gameScreenActive_ = true;
    }
}

void Renderer::leaveGameScreen() const {
    if (!gameScreenActive_) {
        return;
    }
    std::cout << ansi::leaveAlternateScreen();
    gameScreenActive_ = false;
}