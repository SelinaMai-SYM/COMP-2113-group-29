#pragma once

#include <string>
#include <vector>

/**
 * What it does:
 * Enumerates supported in-game command types.
 * Inputs:
 * None.
 * Outputs:
 * A command type consumed by the game controller.
 */
enum class CommandType {
    Invalid,
    Move,
    Restart,
    Help,
    Inventory,
    Panel,
    Use,
    Save,
    Hint,
    Menu,
    Quit
};

/**
 * What it does:
 * Stores one parsed command line.
 * Inputs:
 * None.
 * Outputs:
 * Parsed command metadata and optional indices.
 */
struct Command {
    CommandType type = CommandType::Invalid;
    std::string subject;
    std::vector<int> indices;
    std::string raw;
};

/**
 * What it does:
 * Parses text entered during gameplay.
 * Inputs:
 * Raw user input from one command line.
 * Outputs:
 * A normalized command structure.
 */
class InputParser {
public:
    /**
     * What it does:
     * Parses one command entered by the player.
     * Inputs:
     * The raw line of text from standard input.
     * Outputs:
     * A parsed command structure, or Invalid if the text is not recognized.
     */
    Command parse(const std::string& line) const;
};
