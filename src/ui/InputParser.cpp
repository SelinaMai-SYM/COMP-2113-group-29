#include "ui/InputParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

/*
- What it does:
  Trims leading and trailing whitespace.
- Inputs:
  One text string.
- Outputs:
  The trimmed string.
*/
std::string trimCopy(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

/*
- What it does:
  Converts text to lowercase.
- Inputs:
  One text string.
- Outputs:
  The lowered string.
*/
std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

}  // namespace

Command InputParser::parse(const std::string& line) const {
    Command command;
    command.raw = line;

    const std::string trimmed = trimCopy(line);
    if (trimmed.empty()) {
        return command;
    }

    const std::string lowered = toLowerCopy(trimmed);
    if (lowered == "r" || lowered == "restart") {
        command.type = CommandType::Restart;
        return command;
    }
    if (lowered == "help" || lowered == "h" || lowered == "?") {
        command.type = CommandType::Help;
        return command;
    }
    if (lowered == "i" || lowered == "inv" || lowered == "inventory") {
        command.type = CommandType::Inventory;
        return command;
    }
    if (lowered == "panel") {
        command.type = CommandType::Panel;
        return command;
    }
    if (lowered == "save") {
        command.type = CommandType::Save;
        return command;
    }
    if (lowered == "hint") {
        command.type = CommandType::Hint;
        return command;
    }
    if (lowered == "menu" || lowered == "m") {
        command.type = CommandType::Menu;
        return command;
    }
    if (lowered == "q" || lowered == "quit" || lowered == "exit") {
        command.type = CommandType::Quit;
        return command;
    }

    std::istringstream numberStream(lowered);
    int src = 0;
    int dst = 0;
    if ((numberStream >> src >> dst) && numberStream.eof()) {
        command.type = CommandType::Move;
        command.indices = {src - 1, dst - 1};
        return command;
    }

    std::istringstream stream(lowered);
    std::string word;
    stream >> word;
    if (word == "use") {
        stream >> command.subject;
        if (command.subject.empty()) {
            return command;
        }

        int index = 0;
        while (stream >> index) {
            command.indices.push_back(index - 1);
        }
        command.type = CommandType::Use;
        return command;
    }

    return command;
}
