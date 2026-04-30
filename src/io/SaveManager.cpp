#include "io/SaveManager.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>

#include "core/Mechanics.h"

namespace {

struct ParsedSaveFile {
    GameState state;
    std::string saveName;
    std::time_t savedAt = 0;
};

/*
- What it does:
  Trims whitespace from both ends of one string.
- Inputs:
  The source text.
- Outputs:
  A trimmed copy of the text.
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
  Converts one string to lowercase for stable filename generation.
- Inputs:
  The source text.
- Outputs:
  A lowercase copy.
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
  Checks whether one save-name character is allowed in user-facing names.
- Inputs:
  One character.
- Outputs:
  True for letters, digits, spaces, hyphens and underscores.
*/
bool isAllowedSaveNameChar(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0 || ch == ' ' || ch == '-' || ch == '_';
}

/*
- What it does:
  Builds a filesystem-safe slug for one validated save name.
- Inputs:
  The save display name.
- Outputs:
  A lowercase filename stem using underscores as separators.
*/
std::string slugifySaveName(const std::string& saveName) {
    std::string slug;
    bool previousUnderscore = false;
    for (char ch : toLowerCopy(trimCopy(saveName))) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            slug.push_back(ch);
            previousUnderscore = false;
            continue;
        }
        if (ch == ' ' || ch == '-' || ch == '_') {
            if (!previousUnderscore) {
                slug.push_back('_');
                previousUnderscore = true;
            }
        }
    }

    while (!slug.empty() && slug.front() == '_') {
        slug.erase(slug.begin());
    }
    while (!slug.empty() && slug.back() == '_') {
        slug.pop_back();
    }
    return slug.empty() ? "save" : slug;
}

/*
- What it does:
  Checks whether one path points to an existing regular file.
- Inputs:
  The candidate path.
- Outputs:
  True if the path exists and is a regular file.
*/
bool fileExists(const std::string& path) {
    struct stat info {};
    return ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

/*
- What it does:
  Returns the modification time of one filesystem path.
- Inputs:
  The file path.
- Outputs:
  The path modification time, or zero if unavailable.
*/
std::time_t fileModificationTime(const std::string& path) {
    struct stat info {};
    if (::stat(path.c_str(), &info) != 0) {
        return 0;
    }
    return info.st_mtime;
}

/*
- What it does:
  Checks whether one directory entry name looks like a save file.
- Inputs:
  The entry name.
- Outputs:
  True for visible `.txt` files.
*/
bool looksLikeSaveFileName(const std::string& name) {
    return name.size() > 4 && name != "." && name != ".." &&
           name.substr(name.size() - 4) == ".txt";
}

/*
- What it does:
  Formats one epoch timestamp for save-list display.
- Inputs:
  A UNIX timestamp.
- Outputs:
  A local time string, or a fallback label if unavailable.
*/
std::string formatTimestamp(std::time_t timestamp) {
    if (timestamp <= 0) {
        return "Unknown time";
    }

    std::tm localTime {};
#if defined(_WIN32)
    if (localtime_s(&localTime, &timestamp) != 0) {
        return "Unknown time";
    }
#else
    if (localtime_r(&timestamp, &localTime) == nullptr) {
        return "Unknown time";
    }
#endif

    char buffer[32] = {};
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime) == 0U) {
        return "Unknown time";
    }
    return buffer;
}

/*
- What it does:
  Converts a serialized tube string back into block storage.
- Inputs:
  A compact string where "-" means empty.
- Outputs:
  A vector of blocks from bottom to top.
*/
std::vector<char> deserializeSlots(const std::string& text) {
    if (text == "-") {
        return {};
    }
    std::vector<char> slots(text.begin(), text.end());
    for (char& slot : slots) {
        slot = canonicalColorCode(slot);
    }
    return slots;
}

/*
- What it does:
  Returns a best-move value or -1 when it is missing.
- Inputs:
  The stats record and target difficulty.
- Outputs:
  The recorded value or -1.
*/
int bestMoveOrMissing(const StatsRecord& stats, Difficulty difficulty) {
    const auto it = stats.bestMoves.find(difficulty);
    return (it == stats.bestMoves.end()) ? -1 : it->second;
}

/*
- What it does:
  Reads one named counter from a string-to-int map without mutating it.
- Inputs:
  The source map and the desired key.
- Outputs:
  The stored value, or 0 if the key does not exist.
*/
int getCount(const std::map<std::string, int>& counts, const std::string& key) {
    const auto it = counts.find(key);
    return (it == counts.end()) ? 0 : it->second;
}

/*
- What it does:
  Serializes one vector of boolean flags as a compact 0/1 string.
- Inputs:
  The boolean flags.
- Outputs:
  A compact string, or "-" when the vector is empty.
*/
std::string serializeFlags(const std::vector<bool>& flags) {
    if (flags.empty()) {
        return "-";
    }

    std::string text;
    text.reserve(flags.size());
    for (bool flag : flags) {
        text.push_back(flag ? '1' : '0');
    }
    return text;
}

/*
- What it does:
  Parses a compact 0/1 flag string back into a vector of booleans.
- Inputs:
  The serialized flag text.
- Outputs:
  A vector of parsed boolean flags.
*/
std::vector<bool> deserializeFlags(const std::string& text) {
    if (text == "-") {
        return {};
    }

    std::vector<bool> flags;
    flags.reserve(text.size());
    for (char ch : text) {
        flags.push_back(ch == '1');
    }
    return flags;
}

/*
- What it does:
  Serializes one vector of cover-target color codes using '.' for tubes without targets.
- Inputs:
  The target-color vector.
- Outputs:
  A compact string, or "-" when the vector is empty.
*/
std::string serializeTargets(const std::vector<char>& targets) {
    if (targets.empty()) {
        return "-";
    }

    std::string text;
    text.reserve(targets.size());
    for (char target : targets) {
        text.push_back(target == '\0' ? '.' : canonicalColorCode(target));
    }
    return text;
}

/*
- What it does:
  Parses a compact cover-target string back into canonical color codes.
- Inputs:
  The serialized target string.
- Outputs:
  A vector of target-color codes using '.' for absent targets.
*/
std::vector<char> deserializeTargets(const std::string& text) {
    if (text == "-") {
        return {};
    }

    std::vector<char> targets;
    targets.reserve(text.size());
    for (char ch : text) {
        targets.push_back(ch == '.' ? '.' : canonicalColorCode(ch));
    }
    return targets;
}

/*
- What it does:
  Normalizes one loaded palette vector into canonical color codes.
- Inputs:
  One mutable color-code vector.
- Outputs:
  Legacy codes such as 'O' are rewritten in place.
*/
void normalizeColors(std::vector<char>& colors) {
    for (char& color : colors) {
        color = canonicalColorCode(color);
    }
}

/*
- What it does:
  Chooses a best-effort unlock target for one legacy covered tube.
- Inputs:
  The loaded game state and one covered tube index.
- Outputs:
  The chosen target color, or '.' if none keeps the state usable.
*/
char chooseLegacyCoverTarget(const GameState& state, int tubeIndex) {
    if (tubeIndex < 0 || tubeIndex >= static_cast<int>(state.tubes.size())) {
        return '.';
    }

    const Tube& coveredTube = state.tubes[static_cast<std::size_t>(tubeIndex)];
    for (char color : state.colors) {
        const char canonical = canonicalColorCode(color);
        const bool existsInCoveredTube =
            std::any_of(coveredTube.slots().begin(), coveredTube.slots().end(), [&](char block) {
                return canonicalColorCode(block) == canonical;
            });
        if (existsInCoveredTube) {
            continue;
        }

        bool alreadyCompleted = false;
        for (const Tube& tube : state.tubes) {
            if (tube.isUniformFull() && canonicalColorCode(tube.topColor()) == canonical) {
                alreadyCompleted = true;
                break;
            }
        }
        if (!alreadyCompleted) {
            return canonical;
        }
    }
    return '.';
}

/*
- What it does:
  Restores usable cover targets for legacy saves that predate the new target-color rule.
- Inputs:
  One mutable loaded game state and the originating save version.
- Outputs:
  Legacy covered tubes gain target colors when possible, or unlock to avoid dead states.
*/
void repairLegacyCoverTargets(GameState& state, int version) {
    if (version >= 4) {
        normalizeColors(state.mechanics.obscuredUnlockTargets);
        return;
    }

    state.mechanics.obscuredUnlockTargets.resize(state.tubes.size(), '.');
    for (int index = 0; index < static_cast<int>(state.tubes.size()); ++index) {
        if (!isTubeObscured(state, index)) {
            continue;
        }
        const char target = chooseLegacyCoverTarget(state, index);
        if (target == '.') {
            state.mechanics.obscuredTubes[static_cast<std::size_t>(index)] = false;
            continue;
        }
        state.mechanics.obscuredUnlockTargets[static_cast<std::size_t>(index)] = target;
    }
    state.mechanics.initialObscuredTubes = obscuredTubeCount(state);
}

/*
- What it does:
  Counts how many blocks currently exist across the whole board.
- Inputs:
  The current game state.
- Outputs:
  The total number of stored blocks.
*/
int currentBlockCount(const GameState& state) {
    int count = 0;
    for (const Tube& tube : state.tubes) {
        count += tube.height();
    }
    return count;
}

/*
- What it does:
  Restores the per-level bin-removal counter for newer or legacy saves.
- Inputs:
  One mutable game state and the originating save version.
- Outputs:
  The bin-removal counter is preserved or inferred safely.
*/
void repairLegacyBinCounter(GameState& state, int version) {
    if (version >= 5) {
        return;
    }
    state.removedBlocksByBin = std::max(0, state.expectedBlocks - currentBlockCount(state));
}

/*
- What it does:
  Parses one save file stream into a game state and save metadata.
- Inputs:
  The input stream and an error output string.
- Outputs:
  A parsed save object on success.
*/
std::optional<ParsedSaveFile> parseSaveStream(std::istream& in, std::string& error) {
    ParsedSaveFile parsed;
    GameState& state = parsed.state;
    Difficulty difficulty = Difficulty::Normal;
    std::string key;
    int tubeCount = 0;
    int historyCount = 0;
    int hiddenRowCount = 0;
    int version = 1;

    while (in >> key) {
        if (key == "version") {
            in >> version;
            if (version < 1 || version > 6) {
                error = "Unsupported save version.";
                return std::nullopt;
            }
        } else if (key == "saveName") {
            in >> std::quoted(parsed.saveName);
        } else if (key == "savedAt") {
            long long rawTimestamp = 0;
            in >> rawTimestamp;
            parsed.savedAt = static_cast<std::time_t>(rawTimestamp);
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
        } else if (key == "binRemoved") {
            in >> state.removedBlocksByBin;
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
            normalizeColors(state.colors);
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
                if (version >= 3) {
                    std::string oldSrcHidden;
                    std::string oldDstHidden;
                    int oldSrcObscured = 0;
                    int oldDstObscured = 0;
                    in >> oldSrcHidden >> oldDstHidden >> oldSrcObscured >> oldDstObscured;
                    record.oldSrcHidden = deserializeFlags(oldSrcHidden);
                    record.oldDstHidden = deserializeFlags(oldDstHidden);
                    record.oldSrcObscured = (oldSrcObscured != 0);
                    record.oldDstObscured = (oldDstObscured != 0);
                    if (version >= 4) {
                        std::string oldObscuredAll;
                        in >> oldObscuredAll;
                        record.oldObscuredTubes = deserializeFlags(oldObscuredAll);
                    }
                }
                record.oldSrcSlots = deserializeSlots(oldSrc);
                record.oldDstSlots = deserializeSlots(oldDst);
                state.history.push_back(record);
            }
        } else if (key == "mechanicMeta") {
            int rewardBlocked = 0;
            in >> state.mechanics.initialHiddenBlocks >> state.mechanics.initialObscuredTubes >>
                state.mechanics.deadlocksResolved >> state.mechanics.bonusEmptyTubesGranted >>
                state.mechanics.deadlockPenaltyMoves >> rewardBlocked;
            state.mechanics.rewardBlocked = (rewardBlocked != 0);
        } else if (key == "hiddenRows") {
            in >> hiddenRowCount;
            state.mechanics.hiddenBlocks.clear();
            for (int index = 0; index < hiddenRowCount; ++index) {
                std::string hiddenKey;
                std::string flags;
                in >> hiddenKey >> flags;
                if (hiddenKey != "hidden") {
                    error = "Malformed hidden-block data in save file.";
                    return std::nullopt;
                }
                state.mechanics.hiddenBlocks.push_back(deserializeFlags(flags));
            }
        } else if (key == "obscured") {
            std::string flags;
            in >> flags;
            state.mechanics.obscuredTubes = deserializeFlags(flags);
        } else if (key == "coverTargets") {
            std::string targets;
            in >> targets;
            state.mechanics.obscuredUnlockTargets = deserializeTargets(targets);
        } else {
            error = "Unexpected token in save file: " + key;
            return std::nullopt;
        }
    }

    repairLegacyCoverTargets(state, version);
    repairLegacyBinCounter(state, version);
    syncMechanicState(state);
    return parsed;
}

/*
- What it does:
  Builds one display record from parsed save data.
- Inputs:
  The parsed save, its path, fallback name and legacy marker.
- Outputs:
  A save-slot summary ready for menus.
*/
SaveSlotInfo buildSaveSlotInfo(const ParsedSaveFile& parsed, const std::string& path,
                               const std::string& fallbackName, bool legacySlot) {
    SaveSlotInfo info;
    info.path = path;
    info.displayName = parsed.saveName.empty() ? fallbackName : parsed.saveName;
    info.savedAt = (parsed.savedAt != 0) ? parsed.savedAt : fileModificationTime(path);
    info.savedAtLabel = formatTimestamp(info.savedAt);
    info.difficulty = parsed.state.difficulty;
    info.level = parsed.state.level;
    info.totalLevels = parsed.state.totalLevels;
    info.legacySlot = legacySlot;
    return info;
}

/*
- What it does:
  Orders saves so the newest entries appear first in the list.
- Inputs:
  Two save summary records.
- Outputs:
  True when the left save should appear earlier.
*/
bool newerSaveFirst(const SaveSlotInfo& left, const SaveSlotInfo& right) {
    if (left.savedAt != right.savedAt) {
        return left.savedAt > right.savedAt;
    }
    return left.displayName < right.displayName;
}

}  // namespace

SaveManager::SaveManager(std::string saveDirectoryPath, std::string statsPath,
                         std::string legacySavePath)
    : saveDirectoryPath_(std::move(saveDirectoryPath)),
      statsPath_(std::move(statsPath)),
      legacySavePath_(std::move(legacySavePath)) {}

bool SaveManager::hasSave() const {
    if (fileExists(legacySavePath_)) {
        return true;
    }

    DIR* directory = ::opendir(saveDirectoryPath_.c_str());
    if (directory == nullptr) {
        return false;
    }

    bool found = false;
    while (dirent* entry = ::readdir(directory)) {
        if (looksLikeSaveFileName(entry->d_name)) {
            found = true;
            break;
        }
    }
    ::closedir(directory);
    return found;
}

std::vector<SaveSlotInfo> SaveManager::listSaves() const {
    std::vector<SaveSlotInfo> saves;

    DIR* directory = ::opendir(saveDirectoryPath_.c_str());
    if (directory != nullptr) {
        while (dirent* entry = ::readdir(directory)) {
            const std::string fileName = entry->d_name;
            if (!looksLikeSaveFileName(fileName)) {
                continue;
            }

            const std::string path = saveDirectoryPath_ + "/" + fileName;
            std::ifstream in(path);
            if (!in) {
                continue;
            }

            std::string error;
            const std::optional<ParsedSaveFile> parsed = parseSaveStream(in, error);
            if (!parsed.has_value()) {
                continue;
            }

            saves.push_back(buildSaveSlotInfo(
                *parsed, path, fileName.substr(0, fileName.size() - 4), false));
        }
        ::closedir(directory);
    }

    if (fileExists(legacySavePath_)) {
        std::ifstream in(legacySavePath_);
        if (in) {
            std::string error;
            const std::optional<ParsedSaveFile> parsed = parseSaveStream(in, error);
            if (parsed.has_value()) {
                saves.push_back(buildSaveSlotInfo(*parsed, legacySavePath_,
                                                  "Legacy single-slot save", true));
            }
        }
    }

    std::sort(saves.begin(), saves.end(), newerSaveFirst);
    return saves;
}

std::optional<SaveSlotInfo> SaveManager::findSaveByName(const std::string& saveName) const {
    std::string normalizedName;
    std::string error;
    if (!isValidSaveName(saveName, normalizedName, error)) {
        return std::nullopt;
    }

    const std::string path = savePathForName(normalizedName);
    for (const SaveSlotInfo& save : listSaves()) {
        if (save.path == path) {
            return save;
        }
    }
    return std::nullopt;
}

bool SaveManager::isValidSaveName(const std::string& rawName, std::string& normalizedName,
                                  std::string& error) {
    normalizedName = trimCopy(rawName);
    if (normalizedName.empty()) {
        error = "Save cancelled.";
        return false;
    }

    for (char ch : normalizedName) {
        if (!isAllowedSaveNameChar(ch)) {
            error = "Save names may only use letters, numbers, spaces, '-' and '_'.";
            return false;
        }
    }
    return true;
}

bool SaveManager::saveGame(const GameState& state, const std::string& saveName,
                           std::string& savedPath, std::string& error) const {
    std::string normalizedName;
    if (!isValidSaveName(saveName, normalizedName, error)) {
        return false;
    }

    ensureDataDirectory();

    const std::string resolvedPath = savePathForName(normalizedName);
    std::ofstream out(resolvedPath);
    if (!out) {
        error = "Could not open the save file for writing.";
        return false;
    }

    GameState snapshot = state;
    syncMechanicState(snapshot);
    const long long savedAt = static_cast<long long>(std::time(nullptr));

    out << "version 6\n";
    out << "saveName " << std::quoted(normalizedName) << "\n";
    out << "savedAt " << savedAt << "\n";
    out << "difficulty " << difficultyToString(snapshot.difficulty) << "\n";
    out << "level " << snapshot.level << "\n";
    out << "totalLevels " << snapshot.totalLevels << "\n";
    out << "capacity " << snapshot.capacity << "\n";
    out << "expectedBlocks " << snapshot.expectedBlocks << "\n";
    out << "binRemoved " << snapshot.removedBlocksByBin << "\n";
    out << "moves " << snapshot.moves << "\n";
    out << "totalMoves " << snapshot.totalMoves << "\n";
    out << "baseSeed " << snapshot.baseSeed << "\n";
    out << "currentLevelSeed " << snapshot.currentLevelSeed << "\n";
    out << "inventory " << snapshot.inventory.bin << " " << snapshot.inventory.undo << " "
        << snapshot.inventory.straw << " " << snapshot.inventory.reverse << "\n";
    out << "powerupUses " << getCount(snapshot.powerUpsUsed, "bin") << " "
        << getCount(snapshot.powerUpsUsed, "undo") << " "
        << getCount(snapshot.powerUpsUsed, "straw") << " "
        << getCount(snapshot.powerUpsUsed, "reverse") << "\n";
    out << "powerupRewards " << getCount(snapshot.powerUpRewards, "bin") << " "
        << getCount(snapshot.powerUpRewards, "undo") << " "
        << getCount(snapshot.powerUpRewards, "straw") << " "
        << getCount(snapshot.powerUpRewards, "reverse") << "\n";
    out << "colors " << std::string(snapshot.colors.begin(), snapshot.colors.end()) << "\n";
    out << "tubes " << snapshot.tubes.size() << "\n";
    for (const Tube& tube : snapshot.tubes) {
        out << "tube " << tube.serialize() << "\n";
    }
    out << "history " << snapshot.history.size() << "\n";
    for (const MoveRecord& record : snapshot.history) {
        const std::string oldSrc =
            record.oldSrcSlots.empty() ? "-"
                                       : std::string(record.oldSrcSlots.begin(),
                                                     record.oldSrcSlots.end());
        const std::string oldDst =
            record.oldDstSlots.empty() ? "-"
                                       : std::string(record.oldDstSlots.begin(),
                                                     record.oldDstSlots.end());
        const std::string oldSrcHidden = serializeFlags(record.oldSrcHidden);
        const std::string oldDstHidden = serializeFlags(record.oldDstHidden);
        const std::string oldObscuredAll = serializeFlags(record.oldObscuredTubes);
        out << "record " << record.src << " " << record.dst << " " << record.amount << " "
            << oldSrc << " " << oldDst << " " << oldSrcHidden << " " << oldDstHidden << " "
            << (record.oldSrcObscured ? 1 : 0) << " " << (record.oldDstObscured ? 1 : 0) << " "
            << oldObscuredAll << "\n";
    }
    out << "mechanicMeta " << snapshot.mechanics.initialHiddenBlocks << " "
        << snapshot.mechanics.initialObscuredTubes << " " << snapshot.mechanics.deadlocksResolved
        << " " << snapshot.mechanics.bonusEmptyTubesGranted << " "
        << snapshot.mechanics.deadlockPenaltyMoves << " "
        << (snapshot.mechanics.rewardBlocked ? 1 : 0) << "\n";
    out << "hiddenRows " << snapshot.mechanics.hiddenBlocks.size() << "\n";
    for (const std::vector<bool>& flags : snapshot.mechanics.hiddenBlocks) {
        out << "hidden " << serializeFlags(flags) << "\n";
    }
    out << "obscured " << serializeFlags(snapshot.mechanics.obscuredTubes) << "\n";
    out << "coverTargets " << serializeTargets(snapshot.mechanics.obscuredUnlockTargets) << "\n";

    savedPath = resolvedPath;
    return true;
}

std::optional<GameState> SaveManager::loadGame(const std::string& path, std::string& error) const {
    if (path.empty()) {
        error = "No save file was selected.";
        return std::nullopt;
    }

    std::ifstream in(path);
    if (!in) {
        error = "No save file was found.";
        return std::nullopt;
    }

    std::optional<ParsedSaveFile> parsed = parseSaveStream(in, error);
    if (!parsed.has_value()) {
        return std::nullopt;
    }

    GameState state = parsed->state;
    if (!parsed->saveName.empty()) {
        state.message =
            "Loaded \"" + parsed->saveName + "\" at level " + std::to_string(state.level) + ".";
    } else {
        state.message = "Loaded saved run at level " + std::to_string(state.level) + ".";
    }
    return state;
}

bool SaveManager::deleteSave(const std::string& path, std::string& error) const {
    if (path.empty() || !fileExists(path)) {
        return true;
    }
    if (std::remove(path.c_str()) != 0) {
        error = "Could not remove the old save file.";
        return false;
    }
    return true;
}

bool SaveManager::renameSave(const std::string& oldPath, const std::string& newSaveName,
                             std::string& renamedPath, std::string& error) const {
    if (oldPath.empty()) {
        error = "No save file was selected.";
        return false;
    }

    std::optional<GameState> state = loadGame(oldPath, error);
    if (!state.has_value()) {
        return false;
    }

    if (!saveGame(*state, newSaveName, renamedPath, error)) {
        return false;
    }

    if (oldPath == renamedPath) {
        return true;
    }

    if (!deleteSave(oldPath, error)) {
        error = "The save was written with the new name, but the old file could not be removed.";
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
    ::mkdir(saveDirectoryPath_.c_str(), 0755);
}

std::string SaveManager::savePathForName(const std::string& saveName) const {
    return saveDirectoryPath_ + "/" + slugifySaveName(saveName) + ".txt";
}
