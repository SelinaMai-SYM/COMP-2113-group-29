#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "core/GameState.h"

struct SaveSlotInfo {
    std::string path;
    std::string displayName;
    std::string savedAtLabel;
    Difficulty difficulty = Difficulty::Normal;
    int level = 0;
    int totalLevels = 0;
    std::time_t savedAt = 0;
    bool legacySlot = false;
};

/*
- What it does:
  Handles save files and persistent statistics files.
- Inputs:
  Paths plus game or stats data structures.
- Outputs:
  Serialized files on disk or parsed in-memory objects.
*/
class SaveManager {
public:
    /*
    - What it does:
      Builds the save manager with the chosen save directory and stats file path.
    - Inputs:
      Optional save-directory and statistics file paths.
    - Outputs:
      A persistence helper configured for those locations.
    */
    SaveManager(std::string saveDirectoryPath = "data/saves",
                std::string statsPath = "data/stats.txt",
                std::string legacySavePath = "data/savegame.txt");

    /*
    - What it does:
      Checks whether any saved run currently exists.
    - Inputs:
      None.
    - Outputs:
      True if at least one save file is readable.
    */
    bool hasSave() const;

    /*
    - What it does:
      Lists every readable saved run together with lightweight display metadata.
    - Inputs:
      None.
    - Outputs:
      A save list sorted from newest to oldest.
    */
    std::vector<SaveSlotInfo> listSaves() const;

    /*
    - What it does:
      Looks up one save slot by its display name.
    - Inputs:
      The requested save name.
    - Outputs:
      The matching save metadata when the name resolves to an existing file.
    */
    std::optional<SaveSlotInfo> findSaveByName(const std::string& saveName) const;

    /*
    - What it does:
      Validates one requested save name for named save slots.
    - Inputs:
      The raw user-entered name plus output parameters.
    - Outputs:
      True if the name is valid and writes back the trimmed display name.
    */
    static bool isValidSaveName(const std::string& rawName, std::string& normalizedName,
                                std::string& error);

    /*
    - What it does:
      Serializes the active run to one named save file.
    - Inputs:
      The current game state, chosen save name, saved-path output and error output.
    - Outputs:
      True on success after writing the resolved save path.
    */
    bool saveGame(const GameState& state, const std::string& saveName, std::string& savedPath,
                  std::string& error) const;

    /*
    - What it does:
      Loads one previously saved run from a chosen save path.
    - Inputs:
      The save path and an error string output parameter.
    - Outputs:
      A game state if loading succeeds.
    */
    std::optional<GameState> loadGame(const std::string& path, std::string& error) const;

    /*
    - What it does:
      Deletes one chosen save file.
    - Inputs:
      The save path and an error string output parameter.
    - Outputs:
      True if the file was removed or did not exist.
    */
    bool deleteSave(const std::string& path, std::string& error) const;

    /*
    - What it does:
      Renames one chosen save by rewriting it under a new validated save name.
    - Inputs:
      The old save path, new save name, renamed-path output and error output.
    - Outputs:
      True on success after updating the save metadata and resolving any legacy migration.
    */
    bool renameSave(const std::string& oldPath, const std::string& newSaveName,
                    std::string& renamedPath, std::string& error) const;

    /*
    - What it does:
      Loads persistent aggregate statistics from disk.
    - Inputs:
      None.
    - Outputs:
      A statistics record, or defaults if no file exists.
    */
    StatsRecord loadStats() const;

    /*
    - What it does:
      Saves persistent aggregate statistics to disk.
    - Inputs:
      A statistics record and an error string output parameter.
    - Outputs:
      True on success.
    */
    bool saveStats(const StatsRecord& stats, std::string& error) const;

    /*
    - What it does:
      Produces the base seed for a new run.
    - Inputs:
      None.
    - Outputs:
      A deterministic environment seed or a time-based fallback.
    */
    static unsigned int defaultSeed();

private:
    /*
    - What it does:
      Creates the data and saves directories if they do not already exist.
    - Inputs:
      None.
    - Outputs:
      None.
    */
    void ensureDataDirectory() const;

    /*
    - What it does:
      Resolves one validated save name into its backing save-file path.
    - Inputs:
      The display name chosen by the user.
    - Outputs:
      A stable file path inside the saves directory.
    */
    std::string savePathForName(const std::string& saveName) const;

    std::string saveDirectoryPath_;
    std::string statsPath_;
    std::string legacySavePath_;
};
