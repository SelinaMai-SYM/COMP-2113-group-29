#pragma once

#include <string>
#include <vector>

struct Move {
    int src = -1;
    int dst = -1;
    int amount = 0;
    int score = 0;
    std::string explain;
};

struct MoveRecord {
    int src = -1;
    int dst = -1;
    int amount = 0;
    std::vector<char> oldSrcSlots;
    std::vector<char> oldDstSlots;
};
