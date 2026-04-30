#include "ui/Ansi.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <map>
#include <regex>
#include <thread>

#include <sys/ioctl.h>
#include <unistd.h>

namespace ansi {
namespace {

const std::string kReset = "\033[0m";
const std::regex kAnsiRegex("\033\\[[0-9;]*m");

const std::map<std::string, std::string> kForeground = {
    {"black", "\033[90m"},   {"red", "\033[91m"},     {"green", "\033[92m"},
    {"yellow", "\033[93m"},  {"gold", "\033[93m"},    {"orange", "\033[93m"},
    {"blue", "\033[94m"},    {"magenta", "\033[95m"}, {"violet", "\033[95m"},
    {"pink", "\033[95m"},    {"cyan", "\033[96m"},    {"teal", "\033[96m"},
    {"white", "\033[97m"},   {"gray", "\033[37m"},    {"silver", "\033[37m"},
};

const std::map<std::string, std::string> kBackground = {
    {"black", "\033[40m"},   {"red", "\033[41m"},     {"green", "\033[42m"},
    {"yellow", "\033[43m"},  {"gold", "\033[43m"},    {"orange", "\033[43m"},
    {"blue", "\033[44m"},    {"magenta", "\033[45m"}, {"violet", "\033[45m"},
    {"pink", "\033[45m"},    {"cyan", "\033[46m"},    {"teal", "\033[46m"},
    {"white", "\033[47m"},   {"gray", "\033[100m"},   {"silver", "\033[47m"},
};

const std::map<char, std::string> kCellBackground = {
    {'R', "red"}, {'G', "green"}, {'B', "blue"}, {'Y', "yellow"},
    {'O', "white"}, {'W', "white"}, {'P', "magenta"}, {'C', "cyan"}, {'M', "black"},
};

const std::map<char, std::string> kCellForeground = {
    {'R', "white"}, {'G', "black"}, {'B', "white"}, {'Y', "black"},
    {'O', "black"}, {'W', "black"}, {'P', "white"}, {'C', "black"}, {'M', "white"},
};

/*
- What it does:
  Reads an environment variable as a boolean flag.
- Inputs:
  The variable name.
- Outputs:
  True if the variable is set.
*/
bool hasFlag(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
}

/*
- What it does:
  Checks whether the current terminal can safely receive screen-control sequences.
- Inputs:
  None.
- Outputs:
  True when stdout is a TTY and TERM is not dumb.
*/
bool terminalSupportsScreenControl() {
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    const char* term = std::getenv("TERM");
    return term != nullptr && std::string(term) != "dumb";
}

/*
- What it does:
  Returns how many bytes the next UTF-8 code point occupies.
- Inputs:
  One leading byte.
- Outputs:
  A safe byte count between 1 and 4.
*/
std::size_t utf8CodePointBytes(unsigned char leadByte) {
    if ((leadByte & 0x80U) == 0U) {
        return 1;
    }
    if ((leadByte & 0xE0U) == 0xC0U) {
        return 2;
    }
    if ((leadByte & 0xF0U) == 0xE0U) {
        return 3;
    }
    if ((leadByte & 0xF8U) == 0xF0U) {
        return 4;
    }
    return 1;
}

/*
- What it does:
  Estimates the terminal column width of ANSI-free UTF-8 text.
- Inputs:
  One string with escape codes already stripped or absent.
- Outputs:
  A code-point-based visible width that keeps box-drawing characters aligned.
*/
std::size_t utf8DisplayWidth(const std::string& plainText) {
    std::size_t width = 0;
    for (std::size_t index = 0; index < plainText.size();) {
        const unsigned char leadByte = static_cast<unsigned char>(plainText[index]);
        if (leadByte < 0x20U || (leadByte >= 0x7FU && leadByte < 0xA0U)) {
            ++index;
            continue;
        }
        const std::size_t advance =
            std::min(utf8CodePointBytes(leadByte), plainText.size() - index);
        index += advance;
        ++width;
    }
    return width;
}

}  // namespace

bool supportsColor() {
    if (hasFlag("NO_COLOR")) {
        return false;
    }
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    const char* term = std::getenv("TERM");
    return term != nullptr && std::string(term) != "dumb";
}

bool supportsScreenControl() {
    return terminalSupportsScreenControl() && hasFlag("COLORFUL_TUBE_ALT_SCREEN");
}

bool supportsCursorMotion() {
    return terminalSupportsScreenControl();
}

/*
- What it does:
  Checks whether the game should opt into the terminal alternate screen buffer.
- Inputs:
  None.
- Outputs:
  True only when terminal control is supported and the dedicated flag is set.
*/
bool useAlternateScreen() {
    return supportsScreenControl();
}

std::string style(const std::string& text, const std::string& fg, const std::string& bg,
                  bool bold, bool underline, bool dim) {
    if (!supportsColor()) {
        return text;
    }

    std::string prefix;
    if (!fg.empty()) {
        auto it = kForeground.find(fg);
        if (it != kForeground.end()) {
            prefix += it->second;
        }
    }
    if (!bg.empty()) {
        auto it = kBackground.find(bg);
        if (it != kBackground.end()) {
            prefix += it->second;
        }
    }
    if (bold) {
        prefix += "\033[1m";
    }
    if (underline) {
        prefix += "\033[4m";
    }
    if (dim) {
        prefix += "\033[2m";
    }

    if (prefix.empty()) {
        return text;
    }
    return prefix + text + kReset;
}

std::string cell(char block, int width) {
    const char symbol = (block == '.') ? ' ' : (block == 'M' ? 'D' : block);
    if (!supportsColor()) {
        return "[" + std::string(width, symbol == ' ' ? ' ' : symbol).substr(0, width) + "]";
    }

    std::string payload(width, ' ');
    if (block == '?') {
        std::fill(payload.begin(), payload.end(), '?');
        return "[" + style(payload, "white", "gray", true, false) + "]";
    }
    if (block == '~') {
        std::fill(payload.begin(), payload.end(), '~');
        return "[" + style(payload, "black", "white", true) + "]";
    }
    if (symbol != ' ') {
        const int centerIndex = width / 2;
        payload[centerIndex] = symbol;
    }

    std::string fg = "white";
    std::string bg = "black";
    auto fgIt = kCellForeground.find(block);
    if (fgIt != kCellForeground.end()) {
        fg = fgIt->second;
    }
    auto bgIt = kCellBackground.find(block);
    if (bgIt != kCellBackground.end()) {
        bg = bgIt->second;
    }
    return "[" + style(payload, fg, bg) + "]";
}

std::string clearScreen() {
    if (!supportsScreenControl()) {
        return "";
    }
    return "\033[2J\033[H";
}

std::string enterAlternateScreen() {
    if (!useAlternateScreen()) {
        return "";
    }
    return "\033[?1049h\033[H";
}

std::string leaveAlternateScreen() {
    if (!useAlternateScreen()) {
        return "";
    }
    return "\033[?1049l";
}

std::string moveCursorUp(std::size_t lines) {
    if (lines == 0 || !supportsCursorMotion()) {
        return "";
    }
    return "\033[" + std::to_string(lines) + "A";
}

std::string clearBelow() {
    if (!supportsCursorMotion()) {
        return "";
    }
    return "\033[J";
}

int terminalWidth() {
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
        return static_cast<int>(size.ws_col);
    }
    return 100;
}

void sleepMs(int milliseconds) {
    if (!isatty(STDOUT_FILENO) || hasFlag("NO_ANIMATION")) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::size_t visibleLength(const std::string& text) {
    return utf8DisplayWidth(std::regex_replace(text, kAnsiRegex, ""));
}

std::string padRight(const std::string& text, std::size_t width) {
    const std::size_t length = visibleLength(text);
    if (length >= width) {
        return text;
    }
    return text + std::string(width - length, ' ');
}

std::string center(const std::string& text, std::size_t width) {
    const std::size_t length = visibleLength(text);
    if (length >= width) {
        return text;
    }
    const std::size_t left = (width - length) / 2;
    const std::size_t right = width - length - left;
    return std::string(left, ' ') + text + std::string(right, ' ');
}

std::string repeat(char ch, std::size_t count) {
    return std::string(count, ch);
}

}  // namespace ansi
