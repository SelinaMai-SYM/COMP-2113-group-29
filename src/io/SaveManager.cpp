#include "io/SaveManager.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>

namespace {

/**
 * What it does:
 * Converts a serialized tube string back into block storage.
 * Inputs:
 * A compact string where "-" means empty.
 * Outputs:
 * A vector of blocks from bottom to top.
 */
std::vector<char> deserializeSlots(const std::string& text) {
    if (text == "-") {
        return {};
    }
    return std::vector<char>(text.begin(), text.end());
}

/**
 * What it does:
 * Returns a best-move value or -1 when it is missing.
 * Inputs:
 * The stats record and target difficulty.
 * Outputs:
 * The recorded value or -1.
 */
int bestMoveOrMissing(const StatsRecord& stats, Difficulty difficulty) {
    const auto it = stats.bestMoves.find(difficulty);
    return (it == stats.bestMoves.end()) ? -1 : it->second;
}

/**
 * What it does:
 * Reads one named counter from a string-to-int map without mutating it.
 * Inputs:
 * The source map and the desired key.
 * Outputs:
 * The stored value, or 0 if the key does not exist.
 */
int getCount(const std::map<std::string, int>& counts, const std::string& key) {
    const auto it = counts.find(key);
    return (it == counts.end()) ? 0 : it->second;
}

}  // namespace

SaveManager::SaveManager(std::string savePath, std::string statsPath)
    : savePath_(std::move(savePath)), statsPath_(std::move(statsPath)) {}

bool SaveManager::hasSave() const {
    std::ifstream input(savePath_);
    return input.good();
}

bool SaveManager::saveGame(const GameState& state, std::string& error) const {
    ensureDataDirectory();

    std::ofstream out(savePath_);
    if (!out) {
        error = "Could not open the save file for writing.";
        return false;
    }

    out << "version 2\n";
    out << "difficulty " << difficultyToString(state.difficulty) << "\n";
    out << "level " << state.level << "\n";
    out << "totalLevels " << state.totalLevels << "\n";
    out << "capacity " << state.capacity << "\n";
    out << "expectedBlocks " << state.expectedBlocks << "\n";
    out << "moves " << state.moves << "\n";
    out << "totalMoves " << state.totalMoves << "\n";
    out << "baseSeed " << state.baseSeed << "\n";
    out << "currentLevelSeed " << state.currentLevelSeed << "\n";
    out << "inventory " << state.inventory.bin << " " << state.inventory.undo << " "
        << state.inventory.straw << " " << state.inventory.reverse << "\n";
    out << "powerupUses " << getCount(state.powerUpsUsed, "bin") << " "
        << getCount(state.powerUpsUsed, "undo") << " " << getCount(state.powerUpsUsed, "straw")
        << " " << getCount(state.powerUpsUsed, "reverse") << "\n";
    out << "powerupRewards " << getCount(state.powerUpRewards, "bin") << " "
        << getCount(state.powerUpRewards, "undo") << " "
        << getCount(state.powerUpRewards, "straw") << " "
        << getCount(state.powerUpRewards, "reverse") << "\n";
    out << "colors " << std::string(state.colors.begin(), state.colors.end()) << "\n";
    out << "tubes " << state.tubes.size() << "\n";
    for (const Tube& tube : state.tubes) {
        out << "tube " << tube.serialize() << "\n";
    }
    out << "history " << state.history.size() << "\n";
    for (const MoveRecord& record : state.history) {
        const std::string oldSrc = record.oldSrcSlots.empty()
                                       ? "-"
                                       : std::string(record.oldSrcSlots.begin(), record.oldSrcSlots.end());
        const std::string oldDst = record.oldDstSlots.empty()
                                       ? "-"
                                       : std::string(record.oldDstSlots.begin(), record.oldDstSlots.end());
        out << "record " << record.src << " " << record.dst << " " << record.amount << " "
            << oldSrc << " " << oldDst << "\n";
    }

    return true;
}

std::optional<GameState> SaveManager::loadGame(std::string& error) const {
    std::ifstream in(savePath_);
    if (!in) {
        error = "No save file was found.";
        return std::nullopt;
    }

    GameState state;
    Difficulty difficulty = Difficulty::Normal;
    std::string key;
    int tubeCount = 0;
    int historyCount = 0;
    int version = 1;

    while (in >> key) {
        if (key == "version") {
            in >> version;
            if (version != 1 && version != 2) {
                error = "Unsupported save version.";
                return std::nullopt;
            }
        } else if (key == "difficulty") {
            std::string text;
            in >> text;
            if (!parseDifficulty(text, difficulty)) {
                error = "Invalid difficulty in save file.";
                return std::nullopt;
            }
            state.difficulty = difficulty;
        } else if (key == "level") {
            in >> state.level;
        } else if (key == "totalLevels") {
            in >> state.totalLevels;
        } else if (key == "capacity") {
            in >> state.capacity;
        } else if (key == "expectedBlocks") {
            in >> state.expectedBlocks;
        } else if (key == "moves") {
            in >> state.moves;
        } else if (key == "totalMoves") {
            in >> state.totalMoves;
        } else if (key == "baseSeed") {
            in >> state.baseSeed;
        } else if (key == "currentLevelSeed") {
            in >> state.currentLevelSeed;
        } else if (key == "inventory") {
            in >> state.inventory.bin >> state.inventory.undo >> state.inventory.straw >>
                state.inventory.reverse;
        } else if (key == "powerupUses") {
            in >> state.powerUpsUsed["bin"] >> state.powerUpsUsed["undo"] >>
                state.powerUpsUsed["straw"] >> state.powerUpsUsed["reverse"];
        } else if (key == "powerupRewards") {
            in >> state.powerUpRewards["bin"] >> state.powerUpRewards["undo"] >>
                state.powerUpRewards["straw"] >> state.powerUpRewards["reverse"];
        } else if (key == "colors") {
            std::string colors;
            in >> colors;
            state.colors = std::vector<char>(colors.begin(), colors.end());
        } else if (key == "tubes") {
            in >> tubeCount;
            state.tubes.clear();
            for (int index = 0; index < tubeCount; ++index) {
                std::string tubeKey;
                std::string contents;
                in >> tubeKey >> contents;
                if (tubeKey != "tube") {
                    error = "Malformed tube block in save file.";
                    return std::nullopt;
                }
                state.tubes.emplace_back(state.capacity, deserializeSlots(contents));
            }
        } else if (key == "history") {
            in >> historyCount;
            state.history.clear();
            for (int index = 0; index < historyCount; ++index) {
                std::string recordKey;
                std::string oldSrc;
                std::string oldDst;
                MoveRecord record;
                in >> recordKey >> record.src >> record.dst >> record.amount >> oldSrc >> oldDst;
                if (recordKey != "record") {
                    error = "Malformed undo history in save file.";
                    return std::nullopt;
                }
                record.oldSrcSlots = deserializeSlots(oldSrc);
                record.oldDstSlots = deserializeSlots(oldDst);
                state.history.push_back(record);
            }
        } else {
            error = "Unexpected token in save file: " + key;
            return std::nullopt;
        }
    }

    state.message = "Loaded saved run at level " + std::to_string(state.level) + ".";
    return state;
}

bool SaveManager::deleteSave(std::string& error) const {
    if (!hasSave()) {
        return true;
    }
    if (std::remove(savePath_.c_str()) != 0) {
        error = "Could not remove the old save file.";
        return false;
    }
    return true;
}

StatsRecord SaveManager::loadStats() const {
    std::ifstream in(statsPath_);
    if (!in) {
        return StatsRecord{};
    }

    StatsRecord stats;
    std::string key;
    int version = 1;
    while (in >> key) {
        if (key == "version") {
            in >> version;
            if (version != 1 && version != 2) {
                return StatsRecord{};
            }
        } else if (key == "gamesStarted") {
            in >> stats.gamesStarted;
        } else if (key == "gamesCleared") {
            in >> stats.gamesCleared;
        } else if (key == "totalMovesAcrossWins") {
            in >> stats.totalMovesAcrossWins;
        } else if (key == "powerupUses") {
            in >> stats.powerUpUses["bin"] >> stats.powerUpUses["undo"] >>
                stats.powerUpUses["straw"] >> stats.powerUpUses["reverse"];
        } else if (key == "powerupRewards") {
            in >> stats.powerUpRewards["bin"] >> stats.powerUpRewards["undo"] >>
                stats.powerUpRewards["straw"] >> stats.powerUpRewards["reverse"];
        } else if (key == "bestMoves") {
            int easy = -1;
            int normal = -1;
            int hard = -1;
            in >> easy >> normal >> hard;
            if (easy >= 0) {
                stats.bestMoves[Difficulty::Easy] = easy;
            }
            if (normal >= 0) {
                stats.bestMoves[Difficulty::Normal] = normal;
            }
            if (hard >= 0) {
                stats.bestMoves[Difficulty::Hard] = hard;
            }
        }
    }
    return stats;
}

bool SaveManager::saveStats(const StatsRecord& stats, std::string& error) const {
    ensureDataDirectory();

    std::ofstream out(statsPath_);
    if (!out) {
        error = "Could not open the stats file for writing.";
        return false;
    }

    out << "version 2\n";
    out << "gamesStarted " << stats.gamesStarted << "\n";
    out << "gamesCleared " << stats.gamesCleared << "\n";
    out << "totalMovesAcrossWins " << stats.totalMovesAcrossWins << "\n";
    out << "powerupUses " << getCount(stats.powerUpUses, "bin") << " "
        << getCount(stats.powerUpUses, "undo") << " " << getCount(stats.powerUpUses, "straw")
        << " " << getCount(stats.powerUpUses, "reverse") << "\n";
    out << "powerupRewards " << getCount(stats.powerUpRewards, "bin") << " "
        << getCount(stats.powerUpRewards, "undo") << " "
        << getCount(stats.powerUpRewards, "straw") << " "
        << getCount(stats.powerUpRewards, "reverse") << "\n";
    out << "bestMoves " << bestMoveOrMissing(stats, Difficulty::Easy) << " "
        << bestMoveOrMissing(stats, Difficulty::Normal) << " "
        << bestMoveOrMissing(stats, Difficulty::Hard) << "\n";
    return true;
}

unsigned int SaveManager::defaultSeed() {
    const char* envSeed = std::getenv("SEED");
    if (envSeed != nullptr && envSeed[0] != '\0') {
        return static_cast<unsigned int>(std::strtoul(envSeed, nullptr, 10));
    }

    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return static_cast<unsigned int>(now & 0xffffffffU);
}

void SaveManager::ensureDataDirectory() const {
    ::mkdir("data", 0755);
}
