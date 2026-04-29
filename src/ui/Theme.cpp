#include "ui/Theme.h"

#include "ui/Ansi.h"

namespace theme {

/*
- What it does:
  Returns the accent color name used across highlighted UI elements.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for accent text.
*/
std::string accentColor() {
    return "violet";
}

/*
- What it does:
  Returns the frame color name used for box borders and outlines.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for frame text.
*/
std::string frameColor() {
    return "cyan";
}

/*
- What it does:
  Returns the main title color name used for headings.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for title text.
*/
std::string titleColor() {
    return "magenta";
}

/*
- What it does:
  Returns the default readable text color name.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for standard body text.
*/
std::string textColor() {
    return "white";
}

/*
- What it does:
  Returns the warning color name used for cautionary UI elements.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for warning text.
*/
std::string warningColor() {
    return "orange";
}

/*
- What it does:
  Returns the success color name used for positive results.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for success text.
*/
std::string successColor() {
    return "green";
}

/*
- What it does:
  Returns the muted color name used for secondary UI text.
- Inputs:
  None.
- Outputs:
  The ANSI palette key for muted text.
*/
std::string mutedColor() {
    return "gray";
}

/*
- What it does:
  Builds the shared multi-line ASCII title art shown in menus and intro screens.
- Inputs:
  None.
- Outputs:
  A vector of styled title-art lines ready for rendering.
*/
std::vector<std::string> titleArt() {
    return {
        ansi::style("        .------.  .------.  .------.  .------.            ",
                    frameColor(), "", true),
        ansi::style("        | RRRR |  | GGGG |  | BBBB |  | YYYY |            ",
                    accentColor(), "", true),
        ansi::style("        | RRRR |  | GGGG |  | BBBB |  | YYYY |            ",
                    warningColor(), "", true),
        ansi::style("        '------'  '------'  '------'  '------'            ",
                    successColor(), "", true),
        ansi::style("         ______      __             ______      __        ",
                    titleColor(), "", true),
        ansi::style("        / ____/___  / /___  _______/_  __/_  __/ /_  ___  ",
                    accentColor(), "", true),
        ansi::style("       / /   / __ \\/ / __ \\/ ___/ / / / / / / / __ \\/ _ \\",
                    frameColor(), "", true),
        ansi::style("      / /___/ /_/ / / /_/ / /  / / / / /_/ / /_/ /  __/  ",
                    warningColor(), "", true),
        ansi::style("      \\____/\\____/_/\\____/_/  /_/ /_/\\__,_/_.___/\\___/   ",
                    successColor(), "", true),
        ansi::style("                    COLORFUL_TUBE                     ",
                    textColor(), "", true, false, true),
    };
}

/*
- What it does:
  Builds one bracketed badge with chosen foreground and background colors.
- Inputs:
  The badge text plus the desired foreground and background color names.
- Outputs:
  A styled badge string.
*/
std::string badge(const std::string& text, const std::string& fg, const std::string& bg) {
    return ansi::style("[ " + text + " ]", fg, bg, true);
}

/*
- What it does:
  Returns the styled difficulty badge for one difficulty level.
- Inputs:
  One difficulty enum value.
- Outputs:
  The colored badge string used in menus and HUD panels.
*/
std::string difficultyBadge(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Easy:
            return badge("EASY", "black", "green");
        case Difficulty::Normal:
            return badge("NORMAL", "black", "yellow");
        case Difficulty::Hard:
            return badge("HARD", "white", "red");
    }
    return badge("NORMAL", "black", "yellow");
}

/*
- What it does:
  Returns the badge used for one special mechanic and its resolution state.
- Inputs:
  The mechanic label text and whether it has been resolved.
- Outputs:
  A colored mechanic badge string.
*/
std::string mechanicBadge(const std::string& text, bool resolved) {
    return resolved ? badge(text + " CLEAR", "black", "green")
                    : badge(text, "white", "magenta");
}

/*
- What it does:
  Returns a warning-style badge string.
- Inputs:
  The badge text.
- Outputs:
  A colored warning badge.
*/
std::string warningBadge(const std::string& text) {
    return badge(text, "black", "orange");
}

/*
- What it does:
  Returns a success-style badge string.
- Inputs:
  The badge text.
- Outputs:
  A colored success badge.
*/
std::string successBadge(const std::string& text) {
    return badge(text, "black", "green");
}

/*
- What it does:
  Applies muted styling to secondary explanatory text.
- Inputs:
  The source plain-text string.
- Outputs:
  A dimmed styled string.
*/
std::string muted(const std::string& text) {
    return ansi::style(text, mutedColor(), "", false, false, true);
}

}  // namespace theme
