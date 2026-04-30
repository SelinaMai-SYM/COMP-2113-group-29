#pragma once

#include <string>
#include <vector>

#include "core/GameState.h"

namespace theme {

/*
- What it does:
  Returns the primary accent color used across the themed UI.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string accentColor();

/*
- What it does:
  Returns the frame color used for borders and separators.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string frameColor();

/*
- What it does:
  Returns the title color used for important headings.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string titleColor();

/*
- What it does:
  Returns the default readable text color for the theme.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string textColor();

/*
- What it does:
  Returns the warning color used for danger states and penalties.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string warningColor();

/*
- What it does:
  Returns the success color used for celebratory states.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string successColor();

/*
- What it does:
  Returns the muted color used for softer annotations.
- Inputs:
  None.
- Outputs:
  A stable ANSI color name.
*/
std::string mutedColor();

/*
- What it does:
  Returns the ASCII-art title banner for the fancy terminal menu.
- Inputs:
  None.
- Outputs:
  A vector of themed title lines.
*/
std::vector<std::string> titleArt();

/*
- What it does:
  Builds one compact themed badge.
- Inputs:
  Badge text plus optional foreground/background colors.
- Outputs:
  A styled one-line badge.
*/
std::string badge(const std::string& text, const std::string& fg, const std::string& bg = "");

/*
- What it does:
  Builds a color-coded badge for the chosen difficulty.
- Inputs:
  One difficulty enum.
- Outputs:
  A styled difficulty badge.
*/
std::string difficultyBadge(Difficulty difficulty);

/*
- What it does:
  Builds a badge for one active or resolved special mechanic.
- Inputs:
  A label and whether the mechanic is already resolved.
- Outputs:
  A styled mechanic badge.
*/
std::string mechanicBadge(const std::string& text, bool resolved = false);

/*
- What it does:
  Builds a warning badge for danger states.
- Inputs:
  One short label.
- Outputs:
  A styled warning badge.
*/
std::string warningBadge(const std::string& text);

/*
- What it does:
  Builds a success badge for positive states.
- Inputs:
  One short label.
- Outputs:
  A styled success badge.
*/
std::string successBadge(const std::string& text);

/*
- What it does:
  Applies a softer muted styling to one string.
- Inputs:
  Plain text.
- Outputs:
  A dimmed themed string.
*/
std::string muted(const std::string& text);

}  // namespace theme
