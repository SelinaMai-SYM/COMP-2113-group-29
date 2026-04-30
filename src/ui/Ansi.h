#pragma once

#include <cstddef>
#include <string>

namespace ansi {

/*
- What it does:
  Checks whether the current terminal supports ANSI colors.
- Inputs:
  None.
- Outputs:
  True if color output should be enabled.
*/
bool supportsColor();

/*
- What it does:
  Checks whether full-screen cursor control is enabled for this run.
- Inputs:
  None.
- Outputs:
  True only when the terminal supports advanced control sequences and
  `COLORFUL_TUBE_ALT_SCREEN` is enabled.
*/
bool supportsScreenControl();

/*
- What it does:
  Checks whether the current terminal supports basic cursor-motion sequences.
- Inputs:
  None.
- Outputs:
  True when cursor movement and in-place redraws are safe on the active terminal.
*/
bool supportsCursorMotion();

/*
- What it does:
  Wraps text with optional ANSI foreground, background and style codes.
- Inputs:
  The text plus optional color/style names.
- Outputs:
  A styled string when colors are supported, or the plain text otherwise.
*/
std::string style(const std::string& text, const std::string& fg = "",
                  const std::string& bg = "", bool bold = false, bool underline = false,
                  bool dim = false);

/*
- What it does:
  Renders one block cell for a tube slot.
- Inputs:
  The block identifier and the desired visible width.
- Outputs:
  A colored or plain-text cell string.
*/
std::string cell(char block, int width = 2);

/*
- What it does:
  Returns the ANSI sequence to clear the screen.
- Inputs:
  None.
- Outputs:
  A string that clears the terminal and moves the cursor home when
  screen-control mode is enabled, or an empty string otherwise.
*/
std::string clearScreen();

/*
- What it does:
  Returns the ANSI sequence to switch into an alternate screen buffer.
- Inputs:
  None.
- Outputs:
  A string that activates a dedicated full-screen terminal buffer when
  `COLORFUL_TUBE_ALT_SCREEN` is enabled, or an empty string otherwise.
*/
std::string enterAlternateScreen();

/*
- What it does:
  Returns the ANSI sequence to leave the alternate screen buffer.
- Inputs:
  None.
- Outputs:
  A string that restores the original terminal buffer when alternate-screen
  mode is enabled, or an empty string otherwise.
*/
std::string leaveAlternateScreen();

/*
- What it does:
  Returns the ANSI sequence to move the cursor up by several lines.
- Inputs:
  The number of lines to move upward.
- Outputs:
  A cursor-movement escape sequence when cursor motion is supported,
  or an empty string otherwise.
*/
std::string moveCursorUp(std::size_t lines);

/*
- What it does:
  Returns the ANSI sequence to clear from the cursor to the end of the screen.
- Inputs:
  None.
- Outputs:
  A string that erases the visible region below the cursor when
  cursor motion is supported, or an empty string otherwise.
*/
std::string clearBelow();

/*
- What it does:
  Returns the current terminal width if available.
- Inputs:
  None.
- Outputs:
  The terminal width in columns, or a safe default.
*/
int terminalWidth();

/*
- What it does:
  Sleeps briefly for small text animations.
- Inputs:
  A duration in milliseconds.
- Outputs:
  None.
*/
void sleepMs(int milliseconds);

/*
- What it does:
  Measures the visible width of a string after removing ANSI escape codes.
- Inputs:
  A potentially colored string.
- Outputs:
  The visible display width in terminal columns.
*/
std::size_t visibleLength(const std::string& text);

/*
- What it does:
  Pads a string on the right to a target visible width.
- Inputs:
  The source string and the target width.
- Outputs:
  The padded string.
*/
std::string padRight(const std::string& text, std::size_t width);

/*
- What it does:
  Centers a string inside a target visible width.
- Inputs:
  The source string and the target width.
- Outputs:
  The centered string.
*/
std::string center(const std::string& text, std::size_t width);

/*
- What it does:
  Repeats one character several times.
- Inputs:
  The character and count.
- Outputs:
  A repeated string.
*/
std::string repeat(char ch, std::size_t count);

}  // namespace ansi
