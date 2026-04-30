#pragma once

#include <vector>

#include "core/GameState.h"

/*
- What it does:
  Resizes mechanic metadata so it matches the current number of tubes and blocks.
- Inputs:
  One mutable game state.
- Outputs:
  Hidden-block and obscured-tube arrays are synchronized with the board layout.
*/
void syncMechanicState(GameState& state);

/*
- What it does:
  Reveals any hidden block that is currently exposed at the top of a visible tube.
- Inputs:
  One mutable game state.
- Outputs:
  Newly exposed top blocks become visible to the player.
*/
void revealExposedBlocks(GameState& state);

/*
- What it does:
  Counts how many blocks are still hidden from the player.
- Inputs:
  The current game state.
- Outputs:
  The number of active unknown-color blocks.
*/
int hiddenBlockCount(const GameState& state);

/*
- What it does:
  Counts how many tubes are still fully obscured.
- Inputs:
  The current game state.
- Outputs:
  The number of currently masked tubes.
*/
int obscuredTubeCount(const GameState& state);

/*
- What it does:
  Checks whether one tube is still hidden behind the obscured-tube mechanic.
- Inputs:
  The current game state and one 0-based tube index.
- Outputs:
  True if the tube contents should still be concealed.
*/
bool isTubeObscured(const GameState& state, int tubeIndex);

/*
- What it does:
  Returns the target color tied to one covered tube.
- Inputs:
  The current game state and one 0-based tube index.
- Outputs:
  The target color code, or '.' if that tube has no cover target.
*/
char obscuredTargetColor(const GameState& state, int tubeIndex);

/*
- What it does:
  Returns the character that should be shown for one block position.
- Inputs:
  The current game state, one tube index and one bottom-based slot index.
- Outputs:
  The visible symbol for rendering, which may differ from the true color.
*/
char visibleBlockAt(const GameState& state, int tubeIndex, int slotIndex);

/*
- What it does:
  Removes hidden-state flags from the top of one tube during a regular move.
- Inputs:
  The current game state, one tube index and the number of moved blocks.
- Outputs:
  A vector of moved hidden-state flags from bottom to top.
*/
std::vector<bool> removeTopHiddenFlags(GameState& state, int tubeIndex, int amount);

/*
- What it does:
  Appends moved hidden-state flags onto the top of one destination tube.
- Inputs:
  The current game state, one tube index and the moved flags.
- Outputs:
  The destination hidden-state array is extended.
*/
void appendTopHiddenFlags(GameState& state, int tubeIndex, const std::vector<bool>& flags);

/*
- What it does:
  Removes the hidden-state flag associated with one bottom block.
- Inputs:
  The current game state and one tube index.
- Outputs:
  True on success while writing the removed flag into the output parameter.
*/
bool removeBottomHiddenFlag(GameState& state, int tubeIndex, bool& hidden);

/*
- What it does:
  Appends one hidden-state flag to the top of a tube.
- Inputs:
  The current game state, one tube index and the block visibility flag.
- Outputs:
  The destination hidden-state array gains one element.
*/
void appendTopHiddenFlag(GameState& state, int tubeIndex, bool hidden);

/*
- What it does:
  Reverses the hidden-state order of one tube to match a reversed block order.
- Inputs:
  The current game state and one tube index.
- Outputs:
  The hidden-state array is reversed in place.
*/
void reverseHiddenFlags(GameState& state, int tubeIndex);

/*
- What it does:
  Removes the obscured-tube mask from one tube once the player interacts with it.
- Inputs:
  The current game state and one tube index.
- Outputs:
  True if the tube was previously obscured and is now revealed.
*/
bool liftObscuredTube(GameState& state, int tubeIndex);

/*
- What it does:
  Unlocks any covered tube whose target color has just been completed.
- Inputs:
  One mutable game state and an optional output pointer for the unlocked tube index.
- Outputs:
  True if a covered tube is unlocked successfully.
*/
bool unlockCoveredTubesIfTargetMet(GameState& state, int* unlockedTubeIndex = nullptr);

/*
- What it does:
  Marks one existing block as initially hidden for the unknown-color mechanic.
- Inputs:
  The current game state, one tube index and one bottom-based slot index.
- Outputs:
  True if the block was marked successfully.
*/
bool markHiddenBlock(GameState& state, int tubeIndex, int slotIndex);

/*
- What it does:
  Marks one tube as initially obscured for the whiteout mechanic.
- Inputs:
  The current game state, one tube index and the target color needed to unlock it.
- Outputs:
  True if the tube was marked successfully.
*/
bool markObscuredTube(GameState& state, int tubeIndex, char targetColor);
