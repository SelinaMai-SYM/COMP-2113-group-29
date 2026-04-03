# Color Sort Puzzle Game

## Team Members
- Replace this line with member name and UID.
- Replace this line with member name and UID.
- Replace this line with member name and UID if needed.

## Project Description
`Color Sort Puzzle Game` is a text-based puzzle game for the Linux terminal. The player pours colored blocks between tubes until every tube becomes empty or fully filled with a single color.

This project is inspired by the color sorting puzzle genre and is built with a clean course-project-oriented architecture. Compared with a minimal clone, this version also adds a difficulty system, save/load support, persistent statistics, a hint system, and a more polished ANSI terminal interface.

## Features
- Three difficulties: `Easy`, `Normal`, and `Hard`.
- Fifteen progressively harder levels per run.
- Solvable procedural level generation using deterministic seeds.
- Four power-ups: `bin`, `undo`, `straw`, and `reverse`.
- Lucky-draw reward after each cleared level.
- Save/load support for the current run.
- Persistent statistics across runs.
- Hint system that recommends a legal move.
- ANSI-coloured terminal UI with menu animation, reward spinner, and level-clear confetti.

## Course Requirement Mapping
### 1. Generation of random events
- Random yet reproducible level generation is driven by a base seed.
- A random lucky-draw power-up reward is granted after each cleared level.

### 2. Data structures for storing data
- `std::vector<Tube>` stores the current board.
- `std::vector<MoveRecord>` stores undo history.
- `std::map<std::string, int>` stores persistent and per-run power-up counters.
- Custom structs/classes such as `GameState`, `Inventory`, `Move`, `Tube`, and `StatsRecord` organize the game state clearly.

### 3. Dynamic memory management
- The power-up system is implemented with runtime polymorphism.
- `PowerUpRegistry` owns `std::unique_ptr<PowerUp>` objects, demonstrating safe dynamic memory management.

### 4. File input/output
- `SaveManager` writes the current run to `data/savegame.txt`.
- `SaveManager` also writes persistent statistics to `data/stats.txt`.
- The game can load a previous run and restore the board, inventory, seeds, and undo history.

### 5. Program codes in multiple files
- The project is split into `core`, `ui`, `features`, and `io` modules.
- Each major responsibility is implemented in separate header/source files.

### 6. Multiple difficulty levels
- `Easy` uses more empty tubes and slower scaling.
- `Normal` follows the default balance.
- `Hard` uses denser puzzles and fewer empty tubes in later levels.

## Project Structure
```text
.
|-- main.cpp
|-- Makefile
|-- README.MD
|-- docs/
|   `-- demo-script.md
|-- data/
`-- src/
    |-- core/
    |-- features/
    |-- io/
    `-- ui/
```

## Main Controls
- `1 6`: pour from tube 1 to tube 6
- `r` or `restart`: restart the current level
- `i` or `inv`: view the current inventory in the status panel
- `hint`: request a suggested move
- `save`: save the current run
- `menu`: return to the main menu
- `q` or `quit`: quit the program

### Power-up Commands
- `use bin 3`
- `use undo`
- `use straw 2 3`
- `use reverse 4`

## Non-standard Libraries
- None.

The project only uses the standard library plus normal POSIX terminal headers that are available on the grading environment (`unistd.h`, `sys/ioctl.h`, and `sys/stat.h`).

## Compilation and Execution
Compile from the project root:

```bash
make
```

Run the game:

```bash
./color_tube
```

Optional reproducible run:

```bash
SEED=1330 ./color_tube
```

Clean build files:

```bash
make clean
```

## Notes for Submission
- Fill in the real team-member information before submission.
- Keep your Git commit messages meaningful and non-empty.
- Record a short video that demonstrates: menu, difficulty selection, gameplay, power-ups, save/load, hint, and the statistics page.
- See `docs/team-contribution-plan.md` for a suggested way to divide work and keep the GitHub contribution graph balanced.
