# ♟️ Visual AI Chess Engine (C++17 + Qt6)

A fully-featured Chess application with a custom AI engine built entirely from scratch. It implements advanced chess search algorithms, intelligent heuristics, and core Data Structures and Algorithms (DSA) concepts to simulate strong and efficient gameplay. The engine uses techniques like Minimax search, Alpha-Beta pruning, iterative deepening, and move ordering to deliver competitive AI performance.

Built with C++17 and Qt6, the application provides a smooth and interactive graphical user interface with features like Human vs AI gameplay, move history tracking, undo support, sound effects, and a modern responsive chess board experience.
---

# 📥 Installation (Windows)

**Windows only — no Qt or compiler required.**

1. Go to:

```text
https://github.com/HriditaGhosh/VisualAIChessEngine/releases/latest
```

2. Download:

```text
VisualChessAI_Installer.exe
```

3. Run the installer

4. Follow the setup wizard

5. Launch **Visual AI Chess Engine** from the Desktop or Start Menu

✅ All required Qt6 DLLs, assets, sounds, and resources are bundled.


<p align="center">

[![Watch Demo🎥](https://youtu.be/i88dkWUKs4I?si=D2EXw-WWtvKalvdn)

</p>
---

# 🎮 Features

## 🤖 AI Difficulty Levels

| Level  | Search Depth |
| ------ | ------------ |
| Easy   | 1 Ply        |
| Medium | 3 Ply        |
| Hard   | 5 Ply        |
| Expert | 7 Ply        |

---

## 🎨 User Interface

* Drag & Drop Pieces
* Click-to-Move Controls
* Legal Move Highlights
* Smooth Piece Animations
* Dark / Light Theme Toggle
* Fullscreen Support
* Board Flip Option
* Keyboard Shortcuts

---

## 🔊 Audio

Sound effects for:

* Move
* Capture
* Check
* Castling
* Promotion
* Checkmate

---

## 📜 Game Features

* Move History (Algebraic Notation)
* Undo Move
* Save Game
* Load Game
* Replay Finished Games
* Human vs Human
* Human vs AI

---

# 📖 How To Play

## 🚀 Starting a New Game

1. Open the application
2. Click **New Game**
3. Select:

```text
Human vs AI
```

or

```text
Human vs Human
```

4. Choose AI difficulty
5. Press **Start**

White always moves first.

---

## 🖱️ Making Moves

### Click-To-Move

1. Click a piece
2. Legal moves will be highlighted
3. Click destination square

### Drag & Drop

Simply drag the piece to a legal square.

---

## ⚡ Special Chess Moves

### 🏰 Castling

Move the king two squares toward the rook.

### 👣 En Passant

Capture according to official chess rules when available.

### 👑 Promotion

When a pawn reaches the last rank:

* Queen
* Rook
* Bishop
* Knight

can be selected.

---

# ⌨️ Keyboard Shortcuts

| Shortcut | Action       |
| -------- | ------------ |
| Ctrl + N | New Game     |
| Ctrl + Z | Undo         |
| T        | Toggle Theme |
| S        | Toggle Sound |
| F        | Flip Board   |
| F11      | Fullscreen   |
| Alt + F4 | Exit         |

---

# 🧠 DSA Concepts Used

The AI engine is implemented entirely from scratch using the following Data Structures and Algorithms.

| #  | Concept                          | Purpose                 |
| -- | -------------------------------- | ----------------------- |
| 1  | Recursion + DFS                  | Minimax Search          |
| 2  | Alpha-Beta Pruning               | Branch Elimination      |
| 3  | Iterative Deepening (IDDFS)      | Progressive Deep Search |
| 4  | Hash Table + Dynamic Programming | Transposition Table     |
| 5  | Bit Manipulation (XOR)           | Zobrist Hashing         |
| 6  | Stack                            | Undo System             |
| 7  | 2D Array                         | Board Representation    |
| 8  | Priority Queue / Sorting         | Move Ordering           |
| 9  | Per-Ply Array                    | Killer Move Heuristic   |
| 10 | 2D Hash Table                    | History Heuristic       |
| 11 | Hash Map                         | Opening Book            |
| 12 | Observer Pattern + Queue         | Qt Signals/Slots        |

---

# 🤖 AI Engine

The engine contains:

* Minimax Search
* Alpha-Beta Pruning
* Iterative Deepening
* Principal Variation Search (PVS)
* Transposition Table
* Zobrist Hashing
* Killer Move Heuristic
* History Heuristic
* Opening Book Support
* Move Ordering
* Static Evaluation Function
* Piece-Square Tables

---

# 🏗️ Architecture

```text
MainWindow
│
├── BoardWidget
│
├── MoveHistoryWidget
│
└── ChessController
     │
     ├── Board
     ├── MoveGenerator
     ├── UndoStack
     │
     └── requestAIMove()
             │
             ▼
        AI Worker Thread
             │
             ├── IDDFS
             ├── PVS
             ├── Alpha-Beta
             ├── Transposition Table
             ├── Killer Heuristic
             ├── History Heuristic
             └── Opening Book
             │
             ▼
        moveReady()
             │
             ▼
      SoundManager
      ThemeManager
      PersistenceManager
```

---

# 🛠️ Build From Source

## Requirements

* C++17 Compiler
* CMake 3.16+
* Qt 6.x
* Qt Widgets
* Qt Multimedia

---

## Windows

```bash
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
```

---

## Linux

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev cmake build-essential

mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build . -j$(nproc)

./ChessGUI
```

---

## macOS

```bash
brew install qt6 cmake

export CMAKE_PREFIX_PATH=$(brew --prefix qt6)

mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

make -j$(sysctl -n hw.ncpu)

./ChessGUI
```

---

## Engine Only (No GUI)

```bash
cd engine

g++ -std=c++17 -O2 \
main.cpp \
board.cpp \
movegen.cpp \
evaluator.cpp \
ai_engine.cpp \
game.cpp \
zobrist.cpp \
-o chess_engine

./chess_engine
```

---

# 📁 Project Structure

```text
VisualAIChessEngine/
│
├── src/
│   ├── board/
│   ├── engine/
│   ├── gui/
│   ├── audio/
│   └── persistence/
│
├── assets/
│   ├── pieces/
│   ├── sounds/
│   └── themes/
│
├── resources/
│
├── docs/
│
├── tests/
│
├── CMakeLists.txt
│
└── README.md
```

---

# 📜 License

This project is intended for educational and portfolio purposes.

---

# 👩‍💻 Author

**Hridita Ghosh**

Built with:

* C++17
* Qt6
* Modern Chess AI Techniques
* Advanced DSA Concepts
