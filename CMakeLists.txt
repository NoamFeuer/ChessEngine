cmake_minimum_required(VERSION 3.28)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# Library
add_library(chesscore STATIC
    src/Position/Position.cpp
    src/Position/Piece.cpp
    src/MoveGeneration/GenerateMoves.cpp
    src/MoveGeneration/AttackTables.cpp
    src/MoveGeneration/Magics.cpp
)
target_include_directories(chesscore PUBLIC src)

# Main executable
add_executable(ChessEngine src/main.cpp)
target_link_libraries(ChessEngine PRIVATE chesscore)

# Perft test suite
add_executable(perft_tests tests/perft_tests.cpp)
target_link_libraries(perft_tests PRIVATE chesscore)

add_test(NAME perft_tests COMMAND perft_tests)