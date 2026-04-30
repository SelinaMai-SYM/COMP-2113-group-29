#include "core/Tube.h"

#include <algorithm>

/*
- What it does:
  Builds one default tube with the standard capacity.
- Inputs:
  None.
- Outputs:
  An empty tube object.
*/
Tube::Tube() = default;

/*
- What it does:
  Builds one empty tube with a custom capacity.
- Inputs:
  The maximum number of blocks the tube can hold.
- Outputs:
  An empty tube configured with that capacity.
*/
Tube::Tube(int capacity) : capacity_(capacity) {}

/*
- What it does:
  Builds one tube with predefined contents.
- Inputs:
  The tube capacity and the initial blocks from bottom to top.
- Outputs:
  A tube initialized with the provided slots.
*/
Tube::Tube(int capacity, const std::vector<char>& slots)
    : capacity_(capacity), slots_(slots) {}

/*
- What it does:
  Returns the configured capacity of the tube.
- Inputs:
  None.
- Outputs:
  The maximum number of blocks the tube can store.
*/
int Tube::capacity() const {
    return capacity_;
}

/*
- What it does:
  Reports how many blocks are currently inside the tube.
- Inputs:
  None.
- Outputs:
  The current tube height.
*/
int Tube::height() const {
    return static_cast<int>(slots_.size());
}

/*
- What it does:
  Reports how many additional blocks the tube can still accept.
- Inputs:
  None.
- Outputs:
  The number of free spaces left in the tube.
*/
int Tube::space() const {
    return capacity_ - height();
}

/*
- What it does:
  Checks whether the tube contains no blocks.
- Inputs:
  None.
- Outputs:
  True if the tube is empty.
*/
bool Tube::isEmpty() const {
    return slots_.empty();
}

/*
- What it does:
  Checks whether every stored block has the same color.
- Inputs:
  None.
- Outputs:
  True if the tube is empty or all blocks match.
*/
bool Tube::isUniform() const {
    if (slots_.empty()) {
        return true;
    }
    return std::all_of(slots_.begin(), slots_.end(),
                       [this](char block) { return block == slots_.front(); });
}

/*
- What it does:
  Checks whether the tube is both full and uniform.
- Inputs:
  None.
- Outputs:
  True if the tube is solved completely.
*/
bool Tube::isUniformFull() const {
    return height() == capacity_ && isUniform();
}

/*
- What it does:
  Returns the top block color of the tube.
- Inputs:
  None.
- Outputs:
  The top color, or '.' when the tube is empty.
*/
char Tube::topColor() const {
    return slots_.empty() ? '.' : slots_.back();
}

/*
- What it does:
  Counts the contiguous run of identical blocks at the top.
- Inputs:
  None.
- Outputs:
  The length of the topmost same-color streak.
*/
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

/*
- What it does:
  Exposes read-only access to the stored blocks.
- Inputs:
  None.
- Outputs:
  A constant reference to the bottom-to-top slot vector.
*/
const std::vector<char>& Tube::slots() const {
    return slots_;
}

/*
- What it does:
  Exposes writable access to the stored blocks.
- Inputs:
  None.
- Outputs:
  A mutable reference to the bottom-to-top slot vector.
*/
std::vector<char>& Tube::slots() {
    return slots_;
}

/*
- What it does:
  Converts the tube contents into the save-file representation.
- Inputs:
  None.
- Outputs:
  A string containing the blocks from bottom to top, or '-' if empty.
*/
std::string Tube::serialize() const {
    if (slots_.empty()) {
        return "-";
    }
    return std::string(slots_.begin(), slots_.end());
}
