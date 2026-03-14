# SuperMega Online Chess

A fully featured chess game written in C++17 with an SFML graphical interface and an AI opponent based on the Minimax algorithm with Alpha-Beta pruning.

## Features

- **Full chess rules**: all piece movements, castling (king-side and queen-side), en passant, and pawn promotion with piece choice.
- **AI opponent**: plays as Black using Minimax + Alpha-Beta pruning with configurable search depth (default: 4 plies).
- **Piece-Square Tables**: positional evaluation bonuses encourage the AI to control the center, develop pieces, and keep the king safe.
- **Move ordering**: captures and promotions are searched first to maximize Alpha-Beta pruning efficiency.
- **Graphical interface**: SFML-based board with click-to-move and drag-and-drop support.
- **Visual feedback**: selected piece highlighting, legal move indicators, last move highlight, and check highlighting.

## File Structure

```
CMakeLists.txt          — CMake build configuration
README.md               — This file
src/
  main.cpp              — Entry point, creates GUI and starts game loop
  ChessEngine.h/.cpp    — Board representation, move generation, all chess rules
  ChessBot.h/.cpp       — AI: Evaluate() with PST, Minimax, Alpha-Beta, move ordering
  Gui.h/.cpp            — SFML rendering, piece drawing, mouse input handling
```

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- SFML 2.5+ (`libsfml-dev` on Ubuntu/Debian)

## Building

```bash
# Install SFML (Ubuntu/Debian)
sudo apt-get install libsfml-dev

# Configure and build
mkdir build && cd build
cmake ..
cmake --build . -j4

# Run
./chess
```

## How to Play

- You play as **White** (bottom of the board).
- **Click** a piece to select it — legal moves will be highlighted.
- **Click** a highlighted square to move the piece, or **drag and drop** it.
- When a pawn reaches the last rank, a promotion dialog appears — click a piece to promote.
- After your move, the **AI** (Black) will automatically calculate and play its response.

## AI Configuration

The search depth can be changed in `src/ChessBot.h`:

```cpp
static constexpr int DEFAULT_DEPTH = 4; // change this value
```

Higher depth = stronger play but longer thinking time.

| Depth | Approximate thinking time |
|-------|--------------------------|
| 3     | < 0.5 seconds             |
| 4     | 1–3 seconds               |
| 5     | 5–15 seconds              |