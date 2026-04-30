#pragma once

#include <string>
#include <vector>

/*
- What it does:
  Stores one tube of colored blocks and exposes helper methods for rule checking.
- Inputs:
  The capacity of the tube and, optionally, an initial list of blocks from bottom to top.
- Outputs:
  A tube object that can be queried and modified by the game engine.
*/
class Tube {
public:
    Tube();
    explicit Tube(int capacity);
    Tube(int capacity, const std::vector<char>& slots);

    /*
    - What it does:
      Returns the maximum number of blocks the tube can hold.
    - Inputs:
      None.
    - Outputs:
      The configured capacity.
    */
    int capacity() const;

    /*
    - What it does:
      Returns the current number of blocks in the tube.
    - Inputs:
      None.
    - Outputs:
      The current height.
    */
    int height() const;

    /*
    - What it does:
      Returns the remaining free spaces in the tube.
    - Inputs:
      None.
    - Outputs:
      The number of empty slots.
    */
    int space() const;

    /*
    - What it does:
      Checks whether the tube is empty.
    - Inputs:
      None.
    - Outputs:
      True if the tube has no blocks.
    */
    bool isEmpty() const;

    /*
    - What it does:
      Checks whether every block in the tube has the same color.
    - Inputs:
      None.
    - Outputs:
      True if all blocks are identical or the tube is empty.
    */
    bool isUniform() const;

    /*
    - What it does:
      Checks whether the tube is completely filled with one color.
    - Inputs:
      None.
    - Outputs:
      True if the tube is full and uniform.
    */
    bool isUniformFull() const;

    /*
    - What it does:
      Returns the block at the top of the tube.
    - Inputs:
      None.
    - Outputs:
      The top color, or '.' if the tube is empty.
    */
    char topColor() const;

    /*
    - What it does:
      Counts the contiguous run of the top color.
    - Inputs:
      None.
    - Outputs:
      The number of consecutive top blocks with the same color.
    */
    int topRunLength() const;

    /*
    - What it does:
      Gives read-only access to the tube contents.
    - Inputs:
      None.
    - Outputs:
      The slot list from bottom to top.
    */
    const std::vector<char>& slots() const;

    /*
    - What it does:
      Gives writable access to the tube contents.
    - Inputs:
      None.
    - Outputs:
      The mutable slot list from bottom to top.
    */
    std::vector<char>& slots();

    /*
    - What it does:
      Converts the tube contents to a compact string for save files.
    - Inputs:
      None.
    - Outputs:
      A string containing the blocks from bottom to top, or "-" if empty.
    */
    std::string serialize() const;

private:
    int capacity_ = 4;
    std::vector<char> slots_;
};
