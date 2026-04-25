#pragma once

#include <string>
#include <vector>

class Tube {
public:
    Tube();
    explicit Tube(int capacity);
    Tube(int capacity, const std::vector<char>& slots);

    int capacity() const;
    int height() const;
    int space() const;
    bool isEmpty() const;
    bool isUniform() const;
    bool isUniformFull() const;
    char topColor() const;
    int topRunLength() const;

    const std::vector<char>& slots() const;
    std::vector<char>& slots();
    std::string serialize() const;

private:
    int capacity_ = 4;
    std::vector<char> slots_;
};
