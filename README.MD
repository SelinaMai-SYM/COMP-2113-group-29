# Colorful Tube

## Team Members
Mai Sheung Ying 3036567028\
Cheng Baojing 3036528175\
ZHOU Yujie 3036586892

## Project Description
`Colorful Tube` is a text-based puzzle game for the Linux terminal. The player pours colored blocks between tubes until every non-empty tube are filled with only blocks of the same color.

Compared with a minimal console clone, this project adds a themed ANSI HUD, procedural level generation, save/load, persistent statistics, four power-ups, a hint system, and three extra puzzle mechanics inspired by more advanced sorting games:
- `Unknown colors`: some blocks stay hidden until they are exposed.
- `Whiteout tubes`: some tubes stay sealed under a white cover until the player completes one full target-color tube.
- `Deadlock relief`: if the board becomes unsolved with no legal pours left, the game detects the deadlock automatically, grants one emergency empty tube, and forfeits that level's lucky reward.

## Features
- Three difficulties: `Easy`, `Normal`, and `Hard`.
- Fifteen progressively harder levels per run.
- Deterministic procedural generation driven by a base seed.
- Special-mechanic scaling by level and difficulty.
- Four power-ups: `bin`, `undo`, `straw`, and `reverse`.
- Lucky-draw reward after each cleared level unless deadlock relief was used.
- Named save/load support for the current run with multiple save files.
- Persistent statistics across runs.
- Hint system that prefers moves with better board cleanup and mechanic reveals.
- Fancy ANSI terminal presentation with themed menu art, HUD badges, reward animation, and level-clear confetti.

## Course Requirement Mapping
### 1. Generation of random events
- Level layouts are generated procedurally from deterministic seeds.
- Lucky-draw rewards are chosen randomly after level clears.
- Hidden blocks and whiteout target tubes are placed from randomized candidate positions.

### 2. Data structures for storing data
- `std::vector<Tube>` stores the board.
- `std::vector<MoveRecord>` stores undo history.
- `std::vector<std::vector<bool>>` stores hidden-block state per tube.
- `std::vector<bool>` stores whiteout-tube state.
- `std::map<std::string, int>` stores persistent and per-run power-up counters.
- Custom structs such as `GameState`, `Inventory`, `MechanicState`, `Move`, `Tube`, and `StatsRecord` organize the run state.

### 3. Dynamic memory management
- The power-up system uses runtime polymorphism.
- `PowerUpRegistry` owns `std::unique_ptr<PowerUp>` objects safely.

### 4. File input/output
- `SaveManager` writes named run saves to `data/saves/*.txt` and can still read the legacy `data/savegame.txt` single-slot file.
- `SaveManager` writes aggregate statistics to `data/stats.txt`.
- Save version `6` stores save-name metadata, timestamps, mechanic metadata, hidden-state arrays, whiteout targets, cover state, and undo snapshots.

### 5. Program codes in multiple files
- The project is split into `core`, `ui`, `features`, and `io` modules.
- Core gameplay, special mechanics, rendering theme, persistence, and power-ups are implemented in separate files.

### 6. Multiple difficulty levels
- `Easy`: more empty tubes and later special-mechanic pressure.
- `Normal`: balanced progression with unknown colors appearing earlier.
- `Hard`: denser layouts, fewer empties later, and earlier mechanic escalation.

## Project Structure
```text
.
|-- main.cpp
|-- Makefile
|-- README.MD
|-- data/
`-- src/
    |-- core/
    |   |-- GameController.*
    |   |-- GameState.*
    |   |-- LevelGenerator.*
    |   |-- Mechanics.*
    |   |-- RuleEngine.*
    |   `-- Tube.*
    |-- features/
    |   `-- *PowerUp.*
    |-- io/
    |   `-- SaveManager.*
    `-- ui/
        |-- Ansi.*
        |-- InputParser.*
        |-- Renderer.*
        `-- Theme.*
```

## Main Controls
- `Move`: e.g. `1 6` pours from tube `1` to tube `6`
- `Restart`: `r` or `restart`
- `Inventory`: `i`, `inv`, or `inventory`
- `Hint`: `hint`
- `Help`: `help`, `h`, or `?`
- `Save`: `save`, then enter a save name
- `Load`: main menu `l` / `load`, then choose a saved run by number
- `Manage saves` in the load list: `d N` deletes save `N`, `r N` renames save `N`
- `Menu`: `menu` or `m`
- `Quit`: `q`, `quit`, or `exit`
- `Panel`: `panel` toggles the hidden right-side run sidebar

### Power-up Commands
- `Bin`: `use bin N`
- `Undo`: `use undo`
- `Straw`: `use straw N1 N2`
- `Reverse`: `use reverse N`
- `N`, `N1`, and `N2` mean tube numbers

## Non-standard Libraries
- None.

The project uses only the standard library plus normal POSIX terminal headers available on the grading environment: `unistd.h`, `sys/ioctl.h`, and `sys/stat.h`.

## Compilation and Execution
Compile from the project root:

```bash
make
```

Run the game:

```bash
./colorful_tube
```

Optional reproducible run:

```bash
SEED=2117 ./colorful_tube
```

Optional faster demo run with reduced waiting:

```bash
NO_ANIMATION=1 ./colorful_tube
```

By default the game now stays scrollback-friendly in a normal terminal and
appends new state snapshots instead of redrawing the whole screen.

Optional legacy redraw mode for cleaner screen transitions and animation effects:

```bash
COLORFUL_TUBE_ALT_SCREEN=1 ./colorful_tube
```

Clean build files:

```bash
make clean
```