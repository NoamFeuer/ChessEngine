cmake_minimum_required(VERSION 3.28)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessEngine src/main.cpp
        src/Position/Position.cpp
        src/Position/Position.h
        src/Position/Piece.cpp
        src/Position/Piece.h
        src/MoveGeneration/GenerateMoves.cpp
        src/MoveGeneration/GenerateMoves.h
        src/Position/Move.h
        src/Position/Move.cpp
        src/MoveGeneration/PMD.cpp
        src/MoveGeneration/PMD.h)
