#pragma once
#include <string>

namespace position {
    class Position {
    public:
        constexpr static int WHITE_KINGSIDE = 1;
        constexpr static int WHITE_QUEENSIDE = 2;
        constexpr static int BLACK_KINGSIDE = 4;
        constexpr static int BLACK_QUEENSIDE = 8;

        Position();
        Position(const std::string& fen);
        Position(const Position& position);
        Position& operator=(const Position& position);

        ~Position();


        bool turn() const;
        void flipTurn();
        int castlingRights;
        int enPassantSquare;

        int squares[64];

    private:
        void loadFen(const std::string& fen);
        int fenSquareToIndex(const std::string& square) const;

        int colorToMove;
    };
}
