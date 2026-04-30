#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/GameState.h"
#include "io/SaveManager.h"

/*
- What it does:
  Handles all terminal output, colors, layouts and lightweight animations.
- Inputs:
  Game states, stats and short status strings.
- Outputs:
  Rendered terminal screens for menus, gameplay and result pages.
*/
class Renderer {
public:
    /*
    - What it does:
      Draws the opening splash screen shown once when the program starts.
    - Inputs:
      None.
    - Outputs:
      A full-screen intro with the large title art.
    */
    void renderIntro() const;

    /*
    - What it does:
      Draws the main menu.
    - Inputs:
      Persistent stats and the number of saved runs currently available.
    - Outputs:
      A full menu screen on standard output.
    */
    void renderMenu(const StatsRecord& stats, std::size_t saveCount) const;

    /*
    - What it does:
      Draws the load screen listing every available save slot.
    - Inputs:
      The discovered save summaries and an optional status message.
    - Outputs:
      A load-picker screen on standard output.
    */
    void renderSaveList(const std::vector<SaveSlotInfo>& saves,
                        const std::string& statusMessage = "") const;

    /*
    - What it does:
      Draws the difficulty selection screen.
    - Inputs:
      None.
    - Outputs:
      A difficulty menu on standard output.
    */
    void renderDifficultyMenu() const;

    /*
    - What it does:
      Draws the in-game screen with the board, core sidebar and optional details.
    - Inputs:
      The current game state, whether a save file exists and whether details are expanded.
    - Outputs:
      A full gameplay screen.
    */
    void renderGame(const GameState& state, bool hasSave, bool showDetails) const;

    /*
    - What it does:
      Enters the dedicated terminal screen used for active gameplay.
    - Inputs:
      None.
    - Outputs:
      The game may render inside an alternate terminal buffer.
    */
    void enterGameScreen() const;

    /*
    - What it does:
      Leaves the dedicated gameplay screen and restores the normal terminal buffer.
    - Inputs:
      None.
    - Outputs:
      The terminal returns to the regular screen buffer.
    */
    void leaveGameScreen() const;

    /*
    - What it does:
      Draws the help page.
    - Inputs:
      None.
    - Outputs:
      A help screen.
    */
    void renderHelp() const;

    /*
    - What it does:
      Draws a focused inventory page with power-up counts and usage reminders.
    - Inputs:
      The current game state.
    - Outputs:
      An inventory screen.
    */
    void renderInventory(const GameState& state) const;

    /*
    - What it does:
      Draws the persistent statistics page.
    - Inputs:
      Persistent stats aggregated across runs.
    - Outputs:
      A stats screen.
    */
    void renderStats(const StatsRecord& stats) const;

    /*
    - What it does:
      Draws a level-clear screen with a short confetti animation.
    - Inputs:
      The current game state.
    - Outputs:
      A celebratory result screen.
    */
    void renderLevelClear(const GameState& state) const;

    /*
    - What it does:
      Draws the lucky-draw reveal sequence and final reward card.
    - Inputs:
      The rewarded power-up name.
    - Outputs:
      A short reward reveal animation.
    */
    void renderReward(const std::string& rewardName) const;

    /*
    - What it does:
      Draws the final game-clear screen after the last level.
    - Inputs:
      The final game state.
    - Outputs:
      A summary screen on standard output.
    */
    void renderGameClear(const GameState& state) const;

    /*
    - What it does:
      Waits for the user to press Enter before continuing.
    - Inputs:
      An optional prompt string.
    - Outputs:
      None.
    */
    void waitForEnter(const std::string& prompt = "Press Enter to continue...") const;

private:
    mutable bool gameScreenActive_ = false;
};
