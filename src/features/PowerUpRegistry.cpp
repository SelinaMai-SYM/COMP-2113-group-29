#include "features/PowerUp.h"

#include <random>

PowerUpRegistry::PowerUpRegistry() {
    owned_.push_back(std::make_unique<BinPowerUp>());
    owned_.push_back(std::make_unique<UndoPowerUp>());
    owned_.push_back(std::make_unique<StrawPowerUp>());
    owned_.push_back(std::make_unique<ReversePowerUp>());

    for (const std::unique_ptr<PowerUp>& powerUp : owned_) {
        lookup_[powerUp->name()] = powerUp.get();
    }
}

const PowerUp* PowerUpRegistry::get(const std::string& name) const {
    const auto it = lookup_.find(name);
    return (it == lookup_.end()) ? nullptr : it->second;
}

std::vector<std::string> PowerUpRegistry::names() const {
    std::vector<std::string> result;
    result.reserve(owned_.size());
    for (const std::unique_ptr<PowerUp>& powerUp : owned_) {
        result.push_back(powerUp->name());
    }
    return result;
}

std::string PowerUpRegistry::randomName(std::mt19937& rng) const {
    std::uniform_int_distribution<std::size_t> pick(0, owned_.size() - 1);
    return owned_[pick(rng)]->name();
}
