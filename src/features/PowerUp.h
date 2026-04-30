#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/GameState.h"

/*
- What it does:
  Defines the abstract interface shared by all power-ups.
- Inputs:
  The current game state, power-up arguments and an output message.
- Outputs:
  True on success while mutating the state when appropriate.
*/
class PowerUp {
public:
    /*
    - What it does:
      Destroys one power-up object through the polymorphic base interface.
    - Inputs:
      None.
    - Outputs:
      Any derived resources are released safely.
    */
    virtual ~PowerUp() = default;

    /*
    - What it does:
      Returns the command name used to trigger this power-up.
    - Inputs:
      None.
    - Outputs:
      The stable power-up name.
    */
    virtual std::string name() const = 0;

    /*
    - What it does:
      Returns a short description of the power-up.
    - Inputs:
      None.
    - Outputs:
      A human-readable description.
    */
    virtual std::string description() const = 0;

    /*
    - What it does:
      Applies the power-up to the current game state.
    - Inputs:
      The current game state, 0-based integer arguments and an output message.
    - Outputs:
      True if the power-up was used successfully.
    */
    virtual bool apply(GameState& state, const std::vector<int>& args, std::string& message) const = 0;
};

/*
- What it does:
  Removes the top block from one non-empty tube.
- Inputs:
  The current state and one tube index.
- Outputs:
  True if one block is deleted successfully.
*/
class BinPowerUp : public PowerUp {
public:
    std::string name() const override;
    std::string description() const override;
    bool apply(GameState& state, const std::vector<int>& args, std::string& message) const override;
};

/*
- What it does:
  Restores the previous regular move from the level history.
- Inputs:
  The current state and no tube indices.
- Outputs:
  True if one move is undone successfully.
*/
class UndoPowerUp : public PowerUp {
public:
    std::string name() const override;
    std::string description() const override;
    bool apply(GameState& state, const std::vector<int>& args, std::string& message) const override;
};

/*
- What it does:
  Moves the bottom block of one tube onto another tube.
- Inputs:
  The current state and two tube indices.
- Outputs:
  True if the move succeeds.
*/
class StrawPowerUp : public PowerUp {
public:
    std::string name() const override;
    std::string description() const override;
    bool apply(GameState& state, const std::vector<int>& args, std::string& message) const override;
};

/*
- What it does:
  Reverses the order of blocks inside one incomplete tube.
- Inputs:
  The current state and one tube index.
- Outputs:
  True if the tube can be reversed.
*/
class ReversePowerUp : public PowerUp {
public:
    std::string name() const override;
    std::string description() const override;
    bool apply(GameState& state, const std::vector<int>& args, std::string& message) const override;
};

/*
- What it does:
  Owns all power-up objects and exposes lookup plus random reward selection.
- Inputs:
  None.
- Outputs:
  A registry with dynamically allocated power-up implementations.
*/
class PowerUpRegistry {
public:
    /*
    - What it does:
      Constructs the registry and registers every built-in power-up.
    - Inputs:
      None.
    - Outputs:
      A lookup-ready registry of available power-ups.
    */
    PowerUpRegistry();

    /*
    - What it does:
      Finds one power-up by its command name.
    - Inputs:
      The power-up identifier string.
    - Outputs:
      A pointer to the matching power-up, or nullptr if missing.
    */
    const PowerUp* get(const std::string& name) const;

    /*
    - What it does:
      Returns all registered power-up names.
    - Inputs:
      None.
    - Outputs:
      A vector of names.
    */
    std::vector<std::string> names() const;

    /*
    - What it does:
      Chooses one random power-up reward.
    - Inputs:
      A random generator.
    - Outputs:
      The rewarded power-up name.
    */
    std::string randomName(std::mt19937& rng) const;

private:
    std::vector<std::unique_ptr<PowerUp>> owned_;
    std::unordered_map<std::string, const PowerUp*> lookup_;
};
