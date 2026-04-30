#include "features/PowerUp.h"

#include <random>

/*
- What it does:
  Constructs the power-up registry and indexes every built-in power-up.
- Inputs:
  None.
- Outputs:
  A registry that owns all concrete power-up objects.
*/
PowerUpRegistry::PowerUpRegistry() {
    owned_.push_back(std::make_unique<BinPowerUp>());
    owned_.push_back(std::make_unique<UndoPowerUp>());
    owned_.push_back(std::make_unique<StrawPowerUp>());
    owned_.push_back(std::make_unique<ReversePowerUp>());

    for (const std::unique_ptr<PowerUp>& powerUp : owned_) {
        lookup_[powerUp->name()] = powerUp.get();
    }
}

/*
- What it does:
  Finds one power-up by its stable command name.
- Inputs:
  The requested power-up name.
- Outputs:
  A pointer to the matching power-up, or nullptr if it is absent.
*/
const PowerUp* PowerUpRegistry::get(const std::string& name) const {
    const auto it = lookup_.find(name);
    return (it == lookup_.end()) ? nullptr : it->second;
}

/*
- What it does:
  Returns the list of all registered power-up names.
- Inputs:
  None.
- Outputs:
  A vector containing each power-up identifier once.
*/
std::vector<std::string> PowerUpRegistry::names() const {
    std::vector<std::string> result;
    result.reserve(owned_.size());
    for (const std::unique_ptr<PowerUp>& powerUp : owned_) {
        result.push_back(powerUp->name());
    }
    return result;
}

/*
- What it does:
  Chooses one random reward name from the registered power-ups.
- Inputs:
  A random generator.
- Outputs:
  The selected power-up name.
*/
std::string PowerUpRegistry::randomName(std::mt19937& rng) const {
    std::uniform_int_distribution<std::size_t> pick(0, owned_.size() - 1);
    return owned_[pick(rng)]->name();
}
