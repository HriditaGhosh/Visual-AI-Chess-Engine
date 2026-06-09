cmake_minimum_required(VERSION 3.16)
project(ChessGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg)
find_package(Qt6 OPTIONAL_COMPONENTS Core5Compat)

# ── Engine sources (no main.cpp) ─────────────────────────────
set(ENGINE_SOURCES
    engine/ai_engine.cpp
    engine/board.cpp
    engine/evaluator.cpp
    engine/game.cpp
    engine/movegen.cpp
    engine/zobrist.cpp
)

# ── GUI sources ───────────────────────────────────────────────
set(GUI_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/BoardWidget.cpp
    src/ChessController.cpp
    src/AnimationManager.cpp
    src/SoundManager.cpp
    src/ThemeManager.cpp
    src/MoveHistoryWidget.cpp
    src/PersistenceManager.cpp
    src/StatsManager.cpp
    src/StatsDialog.cpp
    src/ReplayDialog.cpp
    src/ReplayBoardView.cpp
)

set(GUI_HEADERS
    src/MainWindow.h
    src/BoardWidget.h
    src/ChessController.h
    src/AnimationManager.h
    src/SoundManager.h
    src/ThemeManager.h
    src/MoveHistoryWidget.h
    src/PromotionDialog.h
    src/GameSettingsDialog.h
    src/PersistenceManager.h
    src/StatsManager.h
    src/StatsDialog.h
    src/ReplayDialog.h
    src/ReplayBoardView.h
)

add_executable(ChessGUI WIN32
    ${ENGINE_SOURCES}
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    resources/resources.qrc
)

target_include_directories(ChessGUI PRIVATE engine src)

target_link_libraries(ChessGUI PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
    $<$<TARGET_EXISTS:Qt6::Core5Compat>:Qt6::Core5Compat>
)

# Compiler flags
if(MSVC)
    target_compile_options(ChessGUI PRIVATE /W3 /O2)
else()
    target_compile_options(ChessGUI PRIVATE -Wall -Wextra -O2)
endif()