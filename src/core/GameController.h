#pragma once

#include "core/GameState.h"

class GameController {
public:
    GameController();
    void run();

private:
    bool chooseDifficulty(Difficulty& difficulty) const;
    void startNewGame(Difficulty difficulty);
    void playGame(GameState state) const;
    GameState buildStageOneDemoState(Difficulty difficulty) const;
    void printWelcome() const;
    void printState(const GameState& state) const;

    bool running_ = true;
};
