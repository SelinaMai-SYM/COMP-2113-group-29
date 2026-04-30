#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/GameState.h"
#include "features/PowerUp.h"
#include "io/SaveManager.h"
#include "ui/InputParser.h"
#include "ui/Renderer.h"

/*
- What it does:
  Orchestrates menus, gameplay, saving, loading and persistent stats.
- Inputs:
  User input read from standard input.
- Outputs:
  A complete playable game loop on standard output.
*/
class GameController {
public:
    /*
    - What it does:
      Constructs the controller and loads persistent statistics into memory.
    - Inputs:
      None.
    - Outputs:
      A ready-to-run controller instance.
    */
    GameController();

    /*
    - What it does:
      Starts the whole application loop.
    - Inputs:
      None.
    - Outputs:
      None.
    */
    void run();

private:
    enum class SaveListActionType {
        Load,
        Delete,
        Rename,
        Back
    };

    struct SaveListAction {
        SaveListActionType type = SaveListActionType::Back;
        std::size_t index = 0;
    };

    /*
    - What it does:
      Starts a brand-new run with the chosen difficulty.
    - Inputs:
      The selected difficulty.
    - Outputs:
      A playable run until the player returns to the menu or quits.
    */
    void startNewGame(Difficulty difficulty);

    /*
    - What it does:
      Loads one save chosen from the main-menu save list.
    - Inputs:
      The save summaries that are currently available on disk.
    - Outputs:
      None.
    */
    void loadSavedGame(const std::vector<SaveSlotInfo>& availableSaves);

    /*
    - What it does:
      Runs the gameplay loop for one active state.
    - Inputs:
      The current run state.
    - Outputs:
      None.
    */
    void playGame(GameState state);

    /*
    - What it does:
      Shows the difficulty picker and returns the user's choice.
    - Inputs:
      None.
    - Outputs:
      True if the user selected a difficulty, while writing it to the output parameter.
    */
    bool chooseDifficulty(Difficulty& difficulty);

    /*
    - What it does:
      Handles one parsed in-game command.
    - Inputs:
      The current state, parsed command and a flag for returning to the main menu.
    - Outputs:
      None, while mutating the state and controller flags as needed.
    */
    void handleGameCommand(GameState& state, const Command& command, bool& returnToMenu);

    /*
    - What it does:
      Advances the current run after one level is solved.
    - Inputs:
      The solved state.
    - Outputs:
      The next level state, or returns after full game completion.
    */
    void advanceAfterLevelClear(GameState& state);

    /*
    - What it does:
      Finishes the run after the last level is cleared.
    - Inputs:
      The final game state.
    - Outputs:
      None.
    */
    void finishRun(GameState& state);

    /*
    - What it does:
      Applies a named power-up while managing inventory and statistics.
    - Inputs:
      The current state, power-up name and parsed integer arguments.
    - Outputs:
      True if the power-up was consumed successfully.
    */
    bool applyPowerUp(GameState& state, const std::string& name, const std::vector<int>& args);

    /*
    - What it does:
      Prompts for a save name, handles overwrite decisions and persists the run.
    - Inputs:
      The current game state.
    - Outputs:
      True if the save completed successfully.
    */
    bool saveCurrentGame(GameState& state);

    /*
    - What it does:
      Reads one save-name choice from the player.
    - Inputs:
      One mutable output string.
    - Outputs:
      True if the user entered a usable name, or false if the save flow was cancelled.
    */
    bool promptForSaveName(std::string& saveName);

    /*
    - What it does:
      Confirms whether the player wants to overwrite an existing named save.
    - Inputs:
      The conflicting save metadata.
    - Outputs:
      `true` to overwrite, `false` to rename, or `nullopt` to cancel.
    */
    std::optional<bool> confirmOverwrite(const SaveSlotInfo& existingSave);

    /*
    - What it does:
      Confirms whether one selected save should be deleted.
    - Inputs:
      The save metadata.
    - Outputs:
      True if the deletion is confirmed.
    */
    bool confirmDelete(const SaveSlotInfo& selectedSave);

    /*
    - What it does:
      Renames one existing save after prompting for a new name and handling conflicts.
    - Inputs:
      The selected save metadata.
    - Outputs:
      A short status message describing the result.
    */
    std::string renameSaveSlot(const SaveSlotInfo& selectedSave);

    /*
    - What it does:
      Prompts for one action from the load screen.
    - Inputs:
      The available saves and an optional status message from a prior action.
    - Outputs:
      A parsed load/delete/rename/back action.
    */
    SaveListAction chooseSaveAction(const std::vector<SaveSlotInfo>& saves,
                                    const std::string& statusMessage);

    /*
    - What it does:
      Persists the aggregate stats file.
    - Inputs:
      None.
    - Outputs:
      None.
    */
    void persistStats();

    /*
    - What it does:
      Rebuilds the current level from its deterministic seed for restart.
    - Inputs:
      The current state.
    - Outputs:
      A reset copy of the current level with run-wide values preserved.
    */
    GameState rebuildCurrentLevel(const GameState& state) const;

    Renderer renderer_;
    InputParser parser_;
    SaveManager saveManager_;
    PowerUpRegistry powerUps_;
    StatsRecord stats_;
    std::string activeSavePath_;
    bool running_ = true;
    bool detailsPanelExpanded_ = false;
};
