#pragma once

namespace position {
    struct Move {
        int fromSquare;
        int toSquare;
        bool capture;
        bool castle;
        bool enPassant;
        int promotion;

        Move(int fromSquare, int toSquare, bool castle = false, bool enPassant = false, int promotion = 0);
        Move() : fromSquare(0), toSquare(0), capture(false), castle(false), enPassant(false), promotion(0) {}
    };
}
