#pragma once

#include <optional>
#include <string>

#include "core/GameState.h"

/**
 * What it does:
 * Handles save files and persistent statistics files.
 * Inputs:
 * Paths plus game or stats data structures.
 * Outputs:
 * Serialized files on disk or parsed in-memory objects.
 */
class SaveManager {
public:
    SaveManager(std::string savePath = "data/savegame.txt", std::string statsPath = "data/stats.txt");

    /**
     * What it does:
     * Checks whether a saved game currently exists.
     * Inputs:
     * None.
     * Outputs:
     * True if the save file is readable.
     */
    bool hasSave() const;

    /**
     * What it does:
     * Serializes the active run to disk.
     * Inputs:
     * The current game state and an error string output parameter.
     * Outputs:
     * True on success.
     */
    bool saveGame(const GameState& state, std::string& error) const;

    /**
     * What it does:
     * Loads a previously saved run from disk.
     * Inputs:
     * An error string output parameter.
     * Outputs:
     * A game state if loading succeeds.
     */
    std::optional<GameState> loadGame(std::string& error) const;

    /**
     * What it does:
     * Deletes the active save file.
     * Inputs:
     * An error string output parameter.
     * Outputs:
     * True if the file was removed or did not exist.
     */
    bool deleteSave(std::string& error) const;

    /**
     * What it does:
     * Loads persistent aggregate statistics from disk.
     * Inputs:
     * None.
     * Outputs:
     * A statistics record, or defaults if no file exists.
     */
    StatsRecord loadStats() const;

    /**
     * What it does:
     * Saves persistent aggregate statistics to disk.
     * Inputs:
     * A statistics record and an error string output parameter.
     * Outputs:
     * True on success.
     */
    bool saveStats(const StatsRecord& stats, std::string& error) const;

    /**
     * What it does:
     * Produces the base seed for a new run.
     * Inputs:
     * None.
     * Outputs:
     * A deterministic environment seed or a time-based fallback.
     */
    static unsigned int defaultSeed();

private:
    /**
     * What it does:
     * Creates the data directory if it does not already exist.
     * Inputs:
     * None.
     * Outputs:
     * None.
     */
    void ensureDataDirectory() const;

    std::string savePath_;
    std::string statsPath_;
};
