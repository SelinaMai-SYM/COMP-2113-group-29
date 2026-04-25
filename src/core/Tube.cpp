#include "core/Tube.h"

#include <algorithm>

Tube::Tube() = default;

Tube::Tube(int capacity) : capacity_(capacity) {}

Tube::Tube(int capacity, const std::vector<char>& slots)
    : capacity_(capacity), slots_(slots) {}

int Tube::capacity() const {
    return capacity_;
}

int Tube::height() const {
    return static_cast<int>(slots_.size());
}

int Tube::space() const {
    return capacity_ - height();
}

bool Tube::isEmpty() const {
    return slots_.empty();
}

bool Tube::isUniform() const {
    if (slots_.empty()) {
        return true;
    }

    return std::all_of(slots_.begin(), slots_.end(),
                       [this](char block) { return block == slots_.front(); });
}

bool Tube::isUniformFull() const {
    return height() == capacity_ && isUniform();
}

char Tube::topColor() const {
    return slots_.empty() ? '.' : slots_.back();
}

int Tube::topRunLength() const {
    if (slots_.empty()) {
        return 0;
    }

    const char target = slots_.back();
    int count = 0;
    for (auto it = slots_.rbegin(); it != slots_.rend(); ++it) {
        if (*it != target) {
            break;
        }
        ++count;
    }
    return count;
}

const std::vector<char>& Tube::slots() const {
    return slots_;
}

std::vector<char>& Tube::slots() {
    return slots_;
}

std::string Tube::serialize() const {
    if (slots_.empty()) {
        return "-";
    }
    return std::string(slots_.begin(), slots_.end());
}
