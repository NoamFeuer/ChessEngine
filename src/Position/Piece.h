#pragma once

namespace position {
    struct Piece {
        constexpr static int PAWN = 1;
        constexpr static int KNIGHT = 2;
        constexpr static int BISHOP = 3;
        constexpr static int ROOK = 4;
        constexpr static int QUEEN = 5;
        constexpr static int KING = 6;
        constexpr static int NONE = 0;

        constexpr static int BLACK = 8;
        constexpr static int WHITE = 16;

        static int getPieceType(int piece);
        static int getPieceColor(int piece);
        static bool isType(int piece, int type);
        static bool isColor(int piece, int type);
    };
}