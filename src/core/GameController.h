#pragma once

#include "core/GameState.h"
#include "features/PowerUp.h"
#include "io/SaveManager.h"
#include "ui/InputParser.h"
#include "ui/Renderer.h"

/**
 * What it does:
 * Orchestrates menus, gameplay, saving, loading and persistent stats.
 * Inputs:
 * User input read from standard input.
 * Outputs:
 * A complete playable game loop on standard output.
 */
class GameController {
public:
    GameController();

    /**
     * What it does:
     * Starts the whole application loop.
     * Inputs:
     * None.
     * Outputs:
     * None.
     */
    void run();

private:
    /**
     * What it does:
     * Starts a brand-new run with the chosen difficulty.
     * Inputs:
     * The selected difficulty.
     * Outputs:
     * A playable run until the player returns to the menu or quits.
     */
    void startNewGame(Difficulty difficulty);

    /**
     * What it does:
     * Loads a previous save and enters gameplay if loading succeeds.
     * Inputs:
     * None.
     * Outputs:
     * None.
     */
    void loadSavedGame();

    /**
     * What it does:
     * Runs the gameplay loop for one active state.
     * Inputs:
     * The current run state.
     * Outputs:
     * None.
     */
    void playGame(GameState state);

    /**
     * What it does:
     * Shows the difficulty picker and returns the user's choice.
     * Inputs:
     * None.
     * Outputs:
     * True if the user selected a difficulty, while writing it to the output parameter.
     */
    bool chooseDifficulty(Difficulty& difficulty);

    /**
     * What it does:
     * Handles one parsed in-game command.
     * Inputs:
     * The current state, parsed command and a flag for returning to the main menu.
     * Outputs:
     * None, while mutating the state and controller flags as needed.
     */
    void handleGameCommand(GameState& state, const Command& command, bool& returnToMenu);

    /**
     * What it does:
     * Advances the current run after one level is solved.
     * Inputs:
     * The solved state.
     * Outputs:
     * The next level state, or returns after full game completion.
     */
    void advanceAfterLevelClear(GameState& state);

    /**
     * What it does:
     * Finishes the run after the last level is cleared.
     * Inputs:
     * The final game state.
     * Outputs:
     * None.
     */
    void finishRun(GameState& state);

    /**
     * What it does:
     * Applies a named power-up while managing inventory and statistics.
     * Inputs:
     * The current state, power-up name and parsed integer arguments.
     * Outputs:
     * True if the power-up was consumed successfully.
     */
    bool applyPowerUp(GameState& state, const std::string& name, const std::vector<int>& args);

    /**
     * What it does:
     * Persists the aggregate stats file.
     * Inputs:
     * None.
     * Outputs:
     * None.
     */
    void persistStats();

    /**
     * What it does:
     * Rebuilds the current level from its deterministic seed for restart.
     * Inputs:
     * The current state.
     * Outputs:
     * A reset copy of the current level with run-wide values preserved.
     */
    GameState rebuildCurrentLevel(const GameState& state) const;

    Renderer renderer_;
    InputParser parser_;
    SaveManager saveManager_;
    PowerUpRegistry powerUps_;
    StatsRecord stats_;
    bool running_ = true;
    bool detailsPanelExpanded_ = false;
};
