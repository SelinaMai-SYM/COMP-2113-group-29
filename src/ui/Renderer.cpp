#include "ui/Renderer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <vector>

#include <unistd.h>

#include "core/Mechanics.h"
#include "core/RuleEngine.h"
#include "ui/Ansi.h"
#include "ui/Theme.h"

namespace {

const std::string kAccent = "violet";
const std::string kFrame = "cyan";
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

void renderSingleColumnBox(const std::vector<std::string>& lines, std::size_t minWidth);

/*
- What it does:
  Returns the longest visible line length in one panel.
- Inputs:
  One panel line buffer.
- Outputs:
  The maximum ANSI-free visible width.
*/
std::size_t longestVisibleLine(const std::vector<std::string>& lines) {
    std::size_t longest = 0;
    for (const std::string& line : lines) {
        longest = std::max(longest, ansi::visibleLength(line));
    }
    return longest;
}

/*
- What it does:
  Repeats one text fragment several times.
- Inputs:
  The fragment and the repetition count.
- Outputs:
  A concatenated string.
*/
std::string repeatText(const std::string& text, std::size_t count) {
    std::string out;
    for (std::size_t index = 0; index < count; ++index) {
        out += text;
    }
    return out;
}

/*
- What it does:
  Reads one named counter from a string-to-int map without mutating it.
- Inputs:
  The source map and desired key.
- Outputs:
  The stored count or zero when the key is absent.
*/
int getTrackedCount(const std::map<std::string, int>& counts, const std::string& key) {
    const auto it = counts.find(key);
    return (it == counts.end()) ? 0 : it->second;
}

/*
- What it does:
  Splits one plain-text sentence into wrapped lines.
- Inputs:
  The source text and preferred maximum width.
- Outputs:
  A vector of wrapped plain-text lines.
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

/*
- What it does:
  Adds one titled section to a panel.
- Inputs:
  The destination lines, section title and section body.
- Outputs:
  The buffer gains one spaced block.
*/
void appendSection(std::vector<std::string>& lines, const std::string& title,
                   const std::vector<std::string>& body) {
    lines.push_back(ansi::style(title, kAccent, "", true, true));
    lines.insert(lines.end(), body.begin(), body.end());
    lines.push_back("");
}

/*
- What it does:
  Adds one titled section after wrapping each plain-text body line to a target width.
- Inputs:
  The destination lines, section title, section body and wrapping width.
- Outputs:
  The buffer gains one wrapped section block.
*/
void appendWrappedSection(std::vector<std::string>& lines, const std::string& title,
                          const std::vector<std::string>& body, std::size_t width) {
    std::vector<std::string> wrapped;
    for (const std::string& line : body) {
        const std::vector<std::string> partial = wrapText(line, width);
        wrapped.insert(wrapped.end(), partial.begin(), partial.end());
    }
    appendSection(lines, title, wrapped);
}

/*
- What it does:
  Converts a map of power-up counters into printable lines.
- Inputs:
  One string-to-int counter map.
- Outputs:
  A vector of four formatted lines.
*/
std::vector<std::string> powerUpCountLines(const std::map<std::string, int>& counts) {
    std::vector<std::string> lines;
    for (const std::string& name : powerUpNames()) {
        lines.push_back(name + ": " + std::to_string(getTrackedCount(counts, name)));
    }
    return lines;
}

/*
- What it does:
  Formats one saved-run entry into a compact menu line.
- Inputs:
  The save metadata and 0-based list index.
- Outputs:
  One descriptive line for the load screen.
*/
std::string saveListLine(const SaveSlotInfo& save, std::size_t index) {
    std::ostringstream out;
    out << (index + 1) << ". " << save.displayName << " | " << save.savedAtLabel << " | "
        << difficultyLabel(save.difficulty) << " | Level " << save.level << "/" << save.totalLevels;
    if (save.legacySlot) {
        out << " | legacy";
    }
    return out.str();
}

/*
- What it does:
  Converts a reward identifier to uppercase display text.
- Inputs:
  One reward name.
- Outputs:
  An uppercase label for fancy cards and tickers.
*/
std::string uppercaseCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return out;
}

/*
- What it does:
  Chooses one accent color for a specific reward.
- Inputs:
  One reward identifier.
- Outputs:
  A stable ANSI color name.
*/
std::string rewardColor(const std::string& rewardName) {
    if (rewardName == "bin") {
        return theme::warningColor();
    }
    if (rewardName == "undo") {
        return theme::accentColor();
    }
    if (rewardName == "straw") {
        return theme::frameColor();
    }
    if (rewardName == "reverse") {
        return theme::titleColor();
    }
    return theme::successColor();
}

/*
- What it does:
  Checks whether small text animations should run inline on the active terminal.
- Inputs:
  None.
- Outputs:
  True only on interactive terminals when NO_ANIMATION is not set.
*/
bool inlineAnimationsEnabled() {
    const char* value = std::getenv("NO_ANIMATION");
    return isatty(STDOUT_FILENO) && (value == nullptr || value[0] == '\0');
}

/*
- What it does:
  Formats one reward name for the wheel display.
- Inputs:
  The reward identifier and whether the pointer is currently on it.
- Outputs:
  A styled segment label sized for the wheel layout.
*/
std::string buildRewardWheelLabel(const std::string& rewardName, bool selected) {
    const std::string upper = uppercaseCopy(rewardName);
    const std::string text = selected ? ">> " + upper + " <<" : upper;
    return ansi::style(text, rewardColor(rewardName), "", true, false, !selected);
}

/*
- What it does:
  Builds one large wheel-like frame for the lucky-draw reveal.
- Inputs:
  The reward order, the reward currently under the top pointer, a pulse step and lock state.
- Outputs:
  A vector of centered wheel lines ready for rendering.
*/
std::vector<std::string> buildRewardWheelLines(const std::vector<std::string>& rewardNames,
                                               int topIndex, int pulseStep, bool locked) {
    const int count = static_cast<int>(rewardNames.size());
    const std::string topLabel = buildRewardWheelLabel(rewardNames[topIndex % count], true);
    const std::string rightLabel =
        buildRewardWheelLabel(rewardNames[(topIndex + 1) % count], false);
    const std::string bottomLabel =
        buildRewardWheelLabel(rewardNames[(topIndex + 2) % count], false);
    const std::string leftLabel =
        buildRewardWheelLabel(rewardNames[(topIndex + 3) % count], false);

    std::string centerText;
    switch (pulseStep % 4) {
        case 0:
            centerText = "SPINNING";
            break;
        case 1:
            centerText = "SPINNING.";
            break;
        case 2:
            centerText = "SPINNING..";
            break;
        default:
            centerText = "SPINNING...";
            break;
    }
    if (locked) {
        centerText = "POINTER LOCKED";
    }

    const std::string centerLabel = ansi::style(
        centerText, locked ? theme::successColor() : theme::textColor(), "", true);

    std::vector<std::string> lines;
    lines.push_back(ansi::center(theme::badge("LUCKY WHEEL", "black", "yellow"), kMinPageWidth));
    lines.push_back(ansi::center(theme::muted(
                                     locked ? "The wheel stops with the winning reward on top."
                                            : "Watch the top pointer slow down onto the prize."),
                                 kMinPageWidth));
    lines.push_back(ansi::center(ansi::style("v", theme::warningColor(), "", true), kMinPageWidth));
    lines.push_back(ansi::center(".-----------------------.", kMinPageWidth));
    lines.push_back(ansi::center("|" + ansi::center(topLabel, 23) + "|", kMinPageWidth));
    lines.push_back(
        ansi::center(".-----------+-----------------------+-----------.", kMinPageWidth));
    lines.push_back(ansi::center("|" + ansi::center(leftLabel, 11) + "|" +
                                     ansi::center(centerLabel, 23) + "|" +
                                     ansi::center(rightLabel, 11) + "|",
                                 kMinPageWidth));
    lines.push_back(
        ansi::center("'-----------+-----------------------+-----------'", kMinPageWidth));
    lines.push_back(ansi::center("|" + ansi::center(bottomLabel, 23) + "|", kMinPageWidth));
    lines.push_back(ansi::center("'-----------------------'", kMinPageWidth));
    return lines;
}

/*
- What it does:
  Counts how many tubes are complete and uniformly filled.
- Inputs:
  The current game state.
- Outputs:
  The number of complete solved tubes.
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

/*
- What it does:
  Counts how many tubes are empty.
- Inputs:
  The current game state.
- Outputs:
  The number of empty tubes.
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

/*
- What it does:
  Finds the tube index tracked by the covered-tube target mechanic.
- Inputs:
  The current game state.
- Outputs:
  The tracked covered-tube index, or -1 if this level has no cover target.
*/
int trackedCoveredTubeIndex(const GameState& state) {
    for (int index = 0; index < static_cast<int>(state.tubes.size()); ++index) {
        if (obscuredTargetColor(state, index) != '.') {
            return index;
        }
    }
    return -1;
}

/*
- What it does:
  Builds the always-visible target hint for the covered-tube mechanic.
- Inputs:
  The current game state.
- Outputs:
  A short subtitle describing the current cover objective, or an empty string.
*/
std::string coverObjectiveText(const GameState& state) {
    const int coveredTubeIndex = trackedCoveredTubeIndex(state);
    if (coveredTubeIndex < 0) {
        return "";
    }

    const std::string targetLabel = colorLabel(obscuredTargetColor(state, coveredTubeIndex));
    if (isTubeObscured(state, coveredTubeIndex)) {
        return "Target: complete one full " + targetLabel + " tube to unlock covered tube " +
               std::to_string(coveredTubeIndex + 1) + ".";
    }
    return "Target met: covered tube " + std::to_string(coveredTubeIndex + 1) +
           " unlocked after the " + targetLabel + " tube was completed.";
}

/*
- What it does:
  Builds a compact badge strip showing the active mechanics on the current level.
- Inputs:
  The current game state.
- Outputs:
  One styled line combining difficulty and mechanic badges.
*/
std::string mechanicStrip(const GameState& state) {
    std::string line = theme::difficultyBadge(state.difficulty);
    if (state.mechanics.initialHiddenBlocks > 0) {
        line += hiddenBlockCount(state) == 0 ? " " + theme::successBadge("UNKNOWN CLEARED")
                                             : " " + theme::badge("UNKNOWN ?", "white", "gray");
    }
    const int coveredTubeIndex = trackedCoveredTubeIndex(state);
    if (coveredTubeIndex >= 0) {
        line += isTubeObscured(state, coveredTubeIndex)
                    ? " " + theme::badge("COVER LOCKED", "black", "white")
                    : " " + theme::successBadge("COVER UNLOCKED");
    }
    if (state.mechanics.rewardBlocked) {
        line += " " + theme::warningBadge("REWARD LOST");
    }
    return line;
}

/*
- What it does:
  Converts the tube state into a top-down grid for rendering.
- Inputs:
  The current game state.
- Outputs:
  A matrix of block identifiers.
*/
std::vector<std::vector<char>> buildGrid(const GameState& state) {
    std::vector<std::vector<char>> grid(state.capacity,
                                        std::vector<char>(state.tubes.size(), '.'));
    for (std::size_t column = 0; column < state.tubes.size(); ++column) {
        const std::vector<char>& slots = state.tubes[column].slots();
        for (std::size_t index = 0; index < slots.size(); ++index) {
            const int row = state.capacity - 1 - static_cast<int>(index);
            grid[row][column] =
                visibleBlockAt(state, static_cast<int>(column), static_cast<int>(index));
        }
    }
    return grid;
}

/*
- What it does:
  Builds the left board panel containing title, tubes and tube numbers.
- Inputs:
  The current game state plus a custom title and subtitle.
- Outputs:
  A vector of printable strings.
*/
std::vector<std::string> buildBoardLines(const GameState& state, const std::string& title,
                                         const std::string& subtitle = "") {
    std::vector<std::string> lines;
    lines.push_back(ansi::style(title, kTitle, "", true, true));
    lines.push_back(mechanicStrip(state));
    if (!subtitle.empty()) {
        lines.push_back(theme::muted(subtitle));
    }
    lines.push_back("");

    const std::vector<std::vector<char>> grid = buildGrid(state);
    std::ostringstream caps;
    for (std::size_t column = 0; column < state.tubes.size(); ++column) {
        if (column > 0) {
            caps << " ";
        }
        caps << ansi::style("╭──╮", kFrame);
    }
    lines.push_back(caps.str());

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

    std::ostringstream bases;
    for (std::size_t column = 0; column < state.tubes.size(); ++column) {
        if (column > 0) {
            bases << " ";
        }
        bases << ansi::style("╰──╯", kFrame);
    }
    lines.push_back(bases.str());

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

/*
- What it does:
  Builds the in-game sidebar with stats, objectives and optional run details.
- Inputs:
  The current state, save-file availability and detail-toggle state.
- Outputs:
  A vector of printable sidebar lines.
*/
std::vector<std::string> buildSidebarLines(const GameState& state, bool hasSave, bool showDetails) {
    std::vector<std::string> lines;
    const std::size_t legalMoves = RuleEngine::enumerateMoves(state).size();
    const bool deadlocked = RuleEngine::isDeadlocked(state);
    const int coveredTubeIndex = trackedCoveredTubeIndex(state);
    const bool hasSpecialRules = state.mechanics.initialHiddenBlocks > 0 ||
                                 coveredTubeIndex >= 0 ||
                                 state.mechanics.deadlocksResolved > 0 ||
                                 state.mechanics.rewardBlocked;

    lines.push_back(ansi::style("GAME STATUS", kTitle, "", true, true));
    lines.push_back(mechanicStrip(state));
    lines.push_back("");
    appendSection(lines, "RUN",
                  {"Level: " + std::to_string(state.level) + "/" +
                       std::to_string(state.totalLevels),
                   "Moves (level): " + std::to_string(state.moves),
                   "Total moves: " + std::to_string(state.totalMoves),
                   "Saved runs: " + std::string(hasSave ? "available" : "none yet")});
    appendSection(lines, "PRESSURE",
                  {"Complete tubes: " + std::to_string(countCompleteTubes(state)),
                   "Mixed tubes: " + std::to_string(RuleEngine::countMixedTubes(state)),
                   "Empty tubes: " + std::to_string(countEmptyTubes(state)),
                   deadlocked ? "Status: DEADLOCK"
                              : "Status: " + std::to_string(legalMoves) + " legal pours"});
    appendSection(lines, "POWER-UPS",
                  {"bin: " + std::to_string(state.inventory.bin) + " | undo: " +
                       std::to_string(state.inventory.undo),
                   "straw: " + std::to_string(state.inventory.straw) + " | reverse: " +
                       std::to_string(state.inventory.reverse)});

    if (hasSpecialRules) {
        std::vector<std::string> specialLines;
        if (state.mechanics.initialHiddenBlocks > 0) {
            specialLines.push_back("Unknown colors: " +
                                   std::to_string(hiddenBlockCount(state)) + " still hidden");
        }
        if (coveredTubeIndex >= 0) {
            specialLines.push_back("Cover target: complete one full " +
                                   colorLabel(obscuredTargetColor(state, coveredTubeIndex)) +
                                   " tube");
            specialLines.push_back(
                "Covered tube " + std::to_string(coveredTubeIndex + 1) +
                std::string(isTubeObscured(state, coveredTubeIndex) ? " is still locked"
                                                                    : " is unlocked"));
        }
        if (state.mechanics.deadlocksResolved > 0 || state.mechanics.rewardBlocked) {
            specialLines.push_back("Emergency help used: " +
                                   std::to_string(state.mechanics.deadlocksResolved));
            specialLines.push_back("Reward: " +
                                   std::string(state.mechanics.rewardBlocked ? "lost this level"
                                                                            : "still available"));
        }
        appendSection(lines, "SPECIAL RULES", specialLines);
    }

    if (showDetails) {
        appendSection(lines, "TELEMETRY",
                      {"Mode: " + difficultyLabel(state.difficulty),
                       "Base seed: " + std::to_string(state.baseSeed),
                       "Level seed: " + std::to_string(state.currentLevelSeed),
                       "Penalty moves: " +
                           std::to_string(state.mechanics.deadlockPenaltyMoves)});
    }

    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/*
- What it does:
  Builds the sidebar shown on the level-clear screen.
- Inputs:
  The solved level state.
- Outputs:
  A vector of printable summary lines.
*/
std::vector<std::string> buildLevelClearSidebar(const GameState& state) {
    std::vector<std::string> lines;
    lines.push_back(ansi::style("GAME REPORT: LEVEL " + std::to_string(state.level) + " CLEARED",
                                theme::successColor(), "", true, true));
    lines.push_back(mechanicStrip(state));
    lines.push_back("");
    appendSection(lines, "RESULT",
                  {"Moves (this level): " + std::to_string(state.moves),
                   "Total moves: " + std::to_string(state.totalMoves),
                   "Complete tubes: " + std::to_string(countCompleteTubes(state)),
                   "Deadlocks resolved: " +
                       std::to_string(state.mechanics.deadlocksResolved)});
    appendSection(lines, "REWARD",
                  {state.mechanics.rewardBlocked ? "Lucky reward: forfeited by emergency relief"
                                                : "Lucky reward: ready to draw",
                   "Inventory carry-over stays across levels."});
    appendSection(lines, "POWER-UPS",
                  {"bin: " + std::to_string(state.inventory.bin) + " | undo: " +
                       std::to_string(state.inventory.undo),
                   "straw: " + std::to_string(state.inventory.straw) + " | reverse: " +
                       std::to_string(state.inventory.reverse)});
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/*
- What it does:
  Builds the sidebar shown on the final game-clear screen.
- Inputs:
  The final solved game state.
- Outputs:
  A vector of printable run-summary lines.
*/
std::vector<std::string> buildGameClearSidebar(const GameState& state) {
    std::vector<std::string> lines;
    lines.push_back(theme::successBadge("RUN COMPLETE"));
    lines.push_back("Total moves: " + std::to_string(state.totalMoves));
    lines.push_back("Deadlocks resolved: " + std::to_string(state.mechanics.deadlocksResolved));
    lines.push_back("Emergency tubes granted: " +
                    std::to_string(state.mechanics.bonusEmptyTubesGranted));
    lines.push_back("");
    lines.push_back(ansi::style("POWER-UPS USED", kText, "", true));
    for (const auto& name : powerUpNames()) {
        lines.push_back(" " + name + ": " + std::to_string(getTrackedCount(state.powerUpsUsed, name)));
    }
    lines.push_back("");
    lines.push_back(theme::muted("Type R to return to the game menu or Q to quit."));
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

/*
- What it does:
  Prints one styled two-column box with flexible widths.
- Inputs:
  Left and right line buffers.
- Outputs:
  A Unicode-framed layout on standard output.
*/
void renderTwoColumnBox(const std::vector<std::string>& left, const std::vector<std::string>& right) {
    std::size_t leftWidth = std::max(kMinBoardWidth, longestVisibleLine(left));
    std::size_t rightWidth = std::max(kMinSidebarWidth, longestVisibleLine(right));
    const std::size_t requiredInnerWidth = leftWidth + rightWidth + kColumnGapWidth;
    const int detectedWidth = ansi::terminalWidth();
    const std::size_t availableInnerWidth =
        detectedWidth > 2 ? static_cast<std::size_t>(detectedWidth - 2) : requiredInnerWidth;

    if (availableInnerWidth < requiredInnerWidth) {
        renderSingleColumnBox(left, kMinBoardWidth);
        std::cout << "\n";
        renderSingleColumnBox(right, kMinSidebarWidth);
        return;
    }

    const std::size_t targetInnerWidth =
        std::min(kPreferredBoxInnerWidth, availableInnerWidth);

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

/*
- What it does:
  Prints one styled single-column box.
- Inputs:
  One line buffer and a preferred minimum width.
- Outputs:
  A framed single-column layout on standard output.
*/
void renderSingleColumnBox(const std::vector<std::string>& lines, std::size_t minWidth) {
    const int detectedWidth = ansi::terminalWidth();
    const std::size_t availableInnerWidth =
        detectedWidth > 2 ? static_cast<std::size_t>(detectedWidth - 2) : std::max(minWidth, longestVisibleLine(lines));
    const std::size_t innerWidth =
        std::min(std::max(minWidth, longestVisibleLine(lines)), availableInnerWidth);
    std::cout << ansi::style(kTopLeft + repeatText(kHorizontal, innerWidth) + kTopRight, kFrame)
              << "\n";
    for (const std::string& line : lines) {
        std::cout << ansi::style(kVertical, kFrame) << ansi::padRight(line, innerWidth)
                  << ansi::style(kVertical, kFrame) << "\n";
    }
    std::cout << ansi::style(kBottomLeft + repeatText(kHorizontal, innerWidth) + kBottomRight, kFrame)
              << "\n";
}

/*
- What it does:
  Prints the large title art either statically or with a paced reveal.
- Inputs:
  Whether the title should animate and the delay between lines.
- Outputs:
  The themed title art on standard output.
*/
void renderTitleArt(bool animate, int delayMs) {
    const std::vector<std::string> art = theme::titleArt();
    for (const std::string& line : art) {
        std::cout << ansi::center(line, kMenuWidth) << "\n";
        if (animate) {
            ansi::sleepMs(delayMs);
        }
    }
}

/*
- What it does:
  Renders a richer command reference below the gameplay box.
- Inputs:
  None.
- Outputs:
  A multi-line command guide with examples and aliases.
*/
void renderGameGuide() {
    std::cout << ansi::style("COMMANDS", kAccent, "", true, true) << "\n";
    std::cout << " Move: e.g. '1 6' pours from tube 1 to tube 6.\n";
    std::cout << " Restart: 'r' or 'restart'.\n";
    std::cout << " Inventory: 'i', 'inv', or 'inventory'.\n";
    std::cout << " Hint: 'hint'.\n";
    std::cout << " Help: 'help', 'h', or '?'.\n";
    std::cout << " Save: 'save', then enter a save name.\n";
    std::cout << " Menu: 'menu' or 'm'.\n";
    std::cout << " Quit: 'q', 'quit', or 'exit'.\n";
    std::cout << " Power-ups: 'use bin N' | 'use undo' | 'use straw N1 N2' | 'use reverse N'.\n";
    std::cout << " N, N1, and N2 are tube numbers.\n";
    std::cout << " Panel: 'panel' shows or hides the right-side run sidebar.\n";
}

/*
- What it does:
  Builds and prints a small confetti block.
- Inputs:
  A random generator and whether repeated animation frames should be shown.
- Outputs:
  Five rows of celebratory characters.
*/
void renderConfetti(std::mt19937& rng, bool animate, int rows = 5, int columns = 60,
                    int frames = 20, int delayMs = 60, int finalPauseMs = 0) {
    const std::string confettiChars = "*+.: ";
    const std::vector<std::string> colors = {"red", "green", "yellow",
                                             "blue", "magenta", "cyan", "white"};

    auto printFrame = [&]() {
        for (int row = 0; row < rows; ++row) {
            std::string line;
            for (int column = 0; column < columns; ++column) {
                line.push_back(confettiChars[rng() % confettiChars.size()]);
            }
            std::cout << ansi::style(line, colors[rng() % colors.size()]) << "\n";
        }
    };

    if (!animate || frames <= 1) {
        printFrame();
        if (finalPauseMs > 0) {
            ansi::sleepMs(finalPauseMs);
        }
        return;
    }

    for (int frame = 0; frame < frames; ++frame) {
        printFrame();
        ansi::sleepMs(delayMs);
        if (frame < frames - 1) {
            std::cout << ansi::moveCursorUp(static_cast<std::size_t>(rows));
        }
    }
    if (finalPauseMs > 0) {
        ansi::sleepMs(finalPauseMs);
    }
}

}  // namespace

void Renderer::renderIntro() const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n\n";
    renderTitleArt(true, 180);
    std::cout << "\n";
    std::cout << ansi::center(theme::muted("A polished terminal tube sorting puzzle"), kMenuWidth)
              << "\n";
}

void Renderer::renderMenu(const StatsRecord& stats, std::size_t saveCount) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::cout << "\n";
    renderTitleArt(false, 0);
    std::cout << "\n";

    const std::size_t menuWrapWidth = 70;
    std::vector<std::string> lines;
    appendWrappedSection(lines, "GAME",
                         {"Goal: every non-empty tube must end as one full color.",
                          "Clear 15 levels and keep your total moves low.",
                          "Special rules appear naturally when you reach those stages."},
                         menuWrapWidth);
    appendWrappedSection(lines, "ARCHIVE",
                         {"Runs started: " + std::to_string(stats.gamesStarted),
                          "Runs cleared: " + std::to_string(stats.gamesCleared),
                          "Saved runs: " +
                              std::string(saveCount > 0 ? std::to_string(saveCount) + " available"
                                                        : "none yet")},
                         menuWrapWidth);
    appendWrappedSection(lines, "MENU",
                         {"[S] Start game",
                          std::string("[L] Load saved runs") + (saveCount > 0 ? "" : " (empty)"),
                          "[T] View statistics",
                          "[H] Help and controls",
                          "[Q] Quit"},
                         menuWrapWidth);
    renderSingleColumnBox(lines, kMinPageWidth);
    std::cout << "\n> " << std::flush;
}

void Renderer::renderSaveList(const std::vector<SaveSlotInfo>& saves,
                              const std::string& statusMessage) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();

    const std::size_t wrapWidth = 88;
    std::vector<std::string> lines;
    lines.push_back(ansi::style("LOAD SAVED RUN", kTitle, "", true, true));
    lines.push_back(
        theme::muted("Here N means the save number shown at the start of each row."));
    lines.push_back("");
    if (!statusMessage.empty()) {
        appendWrappedSection(lines, "STATUS", {statusMessage}, wrapWidth);
    }

    std::vector<std::string> saveLines;
    saveLines.reserve(saves.size());
    for (std::size_t index = 0; index < saves.size(); ++index) {
        saveLines.push_back(saveListLine(saves[index], index));
    }
    appendWrappedSection(lines, "SAVED RUNS", saveLines, wrapWidth);
    appendSection(lines, "INPUT",
                  {"N = the save number shown at the start of each row.",
                   "Enter N to load save N.",
                   "D N = Delete save N",
                   "R N = Rename save N",
                   "B = Back to the main menu"});
    renderSingleColumnBox(lines, kMinPageWidth);
    std::cout << "\n> " << std::flush;
}

void Renderer::renderDifficultyMenu() const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::vector<std::string> lines;
    lines.push_back(ansi::style("SELECT DIFFICULTY", kTitle, "", true, true));
    lines.push_back(theme::muted("Choose how quickly the game becomes harder."));
    lines.push_back("");
    appendSection(lines, "MODES",
                  {theme::difficultyBadge(Difficulty::Easy) +
                       " more empty tubes, slower pressure",
                   theme::difficultyBadge(Difficulty::Normal) +
                       " balanced progression",
                   theme::difficultyBadge(Difficulty::Hard) +
                       " denser layouts and faster pressure"});
    appendSection(lines, "INPUT",
                  {"1 = Easy", "2 = Normal", "3 = Hard", "B = Back to the main menu"});
    renderSingleColumnBox(lines, kMinPageWidth);
    std::cout << "\n> " << std::flush;
}

void Renderer::renderGame(const GameState& state, bool hasSave, bool showDetails) const {
    leaveGameScreen();
    std::cout << "\n";

    const std::vector<std::string> boardLines =
        buildBoardLines(state, "GAME BOARD", coverObjectiveText(state));
    if (showDetails) {
        renderTwoColumnBox(boardLines, buildSidebarLines(state, hasSave, true));
    } else {
        renderSingleColumnBox(boardLines, kMinPageWidth);
    }
    std::cout << "\n";
    renderGameGuide();
    if (!state.message.empty()) {
        std::cout << "\n";
        std::cout << theme::badge("GAME LOG", theme::textColor(), "") << "\n";
        for (const std::string& line : wrapText(state.message, 92)) {
            std::cout << line << "\n";
        }
    }
    std::cout << "\n> " << std::flush;
}

void Renderer::renderHelp() const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    const std::size_t helpWrapWidth = 70;
    std::vector<std::string> lines;
    lines.push_back(ansi::style("HELP AND GAME CONTROLS", kTitle, "", true, true));
    lines.push_back(theme::muted("Everything stays terminal-safe for SSH and grading."));
    lines.push_back("");
    appendWrappedSection(lines, "QUICK START",
                         {"Compile: make",
                          "Run: ./colorful_tube",
                          "Optional seed: SEED=1330 ./colorful_tube",
                          "Legacy redraw: COLORFUL_TUBE_ALT_SCREEN=1 ./colorful_tube"},
                         helpWrapWidth);
    appendWrappedSection(lines, "CORE INPUT",
                         {"Move: e.g. '1 6' pours from tube 1 to tube 6.",
                          "Restart: 'r' or 'restart'.",
                          "Inventory: 'i', 'inv', or 'inventory'.",
                          "Hint: 'hint'.",
                          "Help: 'help', 'h', or '?'.",
                          "Save: 'save', then enter a save name.",
                          "Menu: 'menu' or 'm'.",
                          "Quit: 'q', 'quit', or 'exit'.",
                          "Panel: 'panel' shows or hides the right-side run sidebar."},
                         helpWrapWidth);
    appendWrappedSection(lines, "SAVE LIST",
                         {"In the load screen, N means the save number shown at the start of each row.",
                          "Enter N to load save N.",
                          "Use 'd N' to delete save N.",
                          "Use 'r N' to rename save N.",
                          "Legacy saves still appear in the list and may be renamed or deleted."},
                         helpWrapWidth);
    appendWrappedSection(lines, "POWER-UPS",
                         {"Bin: 'use bin N'.",
                          "Undo: 'use undo'.",
                          "Straw: 'use straw N1 N2'.",
                          "Reverse: 'use reverse N'.",
                          "N, N1, and N2 are tube numbers."},
                         helpWrapWidth);
    appendWrappedSection(lines, "SPECIAL RULES",
                         {"Unknown colors unlock at Easy L5, Normal L3, and Hard L2, and their count increases as levels go up. Hidden blocks show as gray '?'.",
                          "Covered tubes unlock at Easy L10, Normal L7, and Hard L4. A white cover locks the tube until you complete the shown target color tube.",
                          "If no legal pours remain, the game may give one emergency empty tube and remove that level's reward."},
                         helpWrapWidth);
    renderSingleColumnBox(lines, kMinPageWidth);
}

void Renderer::renderInventory(const GameState& state) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen();
    std::vector<std::string> lines;
    lines.push_back(ansi::style("POWER-UP INVENTORY", kTitle, "", true, true));
    lines.push_back(mechanicStrip(state));
    lines.push_back("");
    appendSection(lines, "STOCK",
                  {"bin: " + std::to_string(state.inventory.bin),
                   "undo: " + std::to_string(state.inventory.undo),
                   "straw: " + std::to_string(state.inventory.straw),
                   "reverse: " + std::to_string(state.inventory.reverse)});
    appendSection(lines, "NOTES",
                  {"Power-ups carry across levels in the same game.",
                   "A new game starts with a fresh inventory.",
                   "Emergency help never gives a replacement reward."});
    renderSingleColumnBox(lines, kMinPageWidth);
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
    lines.push_back(theme::muted("Shared across all runs on this machine."));
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
                     ansi::center("LEVEL " + std::to_string(state.level) + " CLEARED", 60),
                     theme::successColor(), "", true, true)
              << "\n\n";

    std::mt19937 rng(state.currentLevelSeed + 13U);
    const bool animateConfetti = inlineAnimationsEnabled() && ansi::supportsCursorMotion();
    const int confettiColumns =
        std::max(52, std::min(72, ansi::terminalWidth() - 6));
    renderConfetti(rng, animateConfetti, 6, confettiColumns, animateConfetti ? 28 : 1, 85,
                   inlineAnimationsEnabled() ? 320 : 0);
    std::cout << "\n";
    renderTwoColumnBox(buildBoardLines(state, "FINAL GAME STATE"),
                       buildLevelClearSidebar(state));
    std::cout << "\n";
}

void Renderer::renderReward(const std::string& rewardName) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n";

    const std::vector<std::string> rewardNames = powerUpNames();
    auto winnerIt = std::find(rewardNames.begin(), rewardNames.end(), rewardName);
    const int winnerIndex =
        (winnerIt == rewardNames.end()) ? 0 : static_cast<int>(winnerIt - rewardNames.begin());
    const bool animateWheel = inlineAnimationsEnabled() && ansi::supportsCursorMotion();
    const int wheelSteps =
        animateWheel ? static_cast<int>(rewardNames.size()) * 6 + winnerIndex + 1 : 1;

    std::size_t previousWheelLines = 0;
    for (int step = 0; step < wheelSteps; ++step) {
        const int topIndex = animateWheel ? (step % static_cast<int>(rewardNames.size()))
                                          : winnerIndex;
        const bool locked = step == wheelSteps - 1;
        const std::vector<std::string> wheelLines =
            buildRewardWheelLines(rewardNames, topIndex, step, locked);

        if (animateWheel && previousWheelLines > 0U) {
            std::cout << ansi::moveCursorUp(previousWheelLines) << ansi::clearBelow();
        }
        for (const std::string& line : wheelLines) {
            std::cout << line << "\n";
        }
        std::cout << std::flush;
        previousWheelLines = wheelLines.size();

        if (animateWheel) {
            if (step < wheelSteps - 8) {
                ansi::sleepMs(80);
            } else if (step < wheelSteps - 4) {
                ansi::sleepMs(150);
            } else {
                ansi::sleepMs(260);
            }
        }
    }
    std::cout << "\n";
    if (animateWheel) {
        ansi::sleepMs(380);
    }

    const std::string upper = uppercaseCopy(rewardName);
    const std::size_t rewardCardWidth = 66;
    std::vector<std::string> lines;
    lines.push_back(theme::successBadge("LUCKY DRAW COMPLETE"));
    lines.push_back("");
    lines.push_back(ansi::style(ansi::center("POINTER STOPS ON THE WINNING REWARD",
                                             rewardCardWidth),
                                theme::successColor(), "", true));
    lines.push_back("");
    lines.push_back(ansi::style(ansi::center(">>> " + upper + " <<<", rewardCardWidth - 2),
                                rewardColor(rewardName), "", true));
    lines.push_back(ansi::style(ansi::center("Added to your inventory immediately.",
                                             rewardCardWidth),
                                kText, "", true));
    lines.push_back(theme::muted(ansi::center("Keep it for any level later in this run.",
                                              rewardCardWidth)));
    lines.push_back(theme::muted(ansi::center("The wheel reveal works the same across all modes.",
                                              rewardCardWidth)));
    renderSingleColumnBox(lines, rewardCardWidth);
    std::cout << "\nPress Enter to continue to next level...\n";
}

void Renderer::renderGameClear(const GameState& state) const {
    leaveGameScreen();
    std::cout << ansi::clearScreen() << "\n";
    std::cout << ansi::style("THE GAME RUN IS COMPLETE", theme::successColor(), "", true, true)
              << "\n\n";
    renderSingleColumnBox(buildGameClearSidebar(state), 60);
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
