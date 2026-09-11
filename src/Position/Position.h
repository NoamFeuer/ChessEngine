#pragma once
#include <string>
#include <cstdint>
#include "Bitboard.h"

namespace position {
    class Position {
    public:
        constexpr static int WHITE_KINGSIDE = 1;
        constexpr static int WHITE_QUEENSIDE = 2;
        constexpr static int BLACK_KINGSIDE = 4;
        constexpr static int BLACK_QUEENSIDE = 8;

        Position();
        Position(const std::string& fen);
        Position(const Position&) = default;
        Position& operator=(const Position&) = default;

        bool turn() const;
        void flipTurn();

        int castlingRights;
        int enPassantSquare;

        Bitboard byColor[2];   // [0] = white, [1] = black
        Bitboard byType[7];    // indexed by Piece::PAWN..KING (1..6), [0] unused

        Bitboard occupancy() const { return byColor[0] | byColor[1]; }
        Bitboard pieces(Bitboard colorBB, int type) const { return colorBB & byType[type]; }

        int whiteKingSquare() const { return lsb(byColor[0] & byType[6]); }
        int blackKingSquare() const { return lsb(byColor[1] & byType[6]); }
        int kingSquare(int colorIndex) const;

        static int colorToIndex(Bitboard colorMask);

    private:
        void loadFen(const std::string& fen);
        int fenSquareToIndex(const std::string& square) const;

        int colorToMove; // 0 = white, 1 = black
    };
}