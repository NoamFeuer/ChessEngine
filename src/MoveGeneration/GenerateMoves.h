#pragma once
#include <cstdint>
#include <vector>
#include "../Position/Move.h"
#include "../Position/Position.h"

namespace moveGeneration {
    class MoveGenerator {
    public:
        static std::vector<position::Move> generatePseudoLegalMoves(const position::Position& pos);
        static std::vector<position::Move> generateLegalMoves(const position::Position& pos);

        static std::uint64_t perft(const position::Position& pos, int depth);
        static void applyMove(position::Position& pos, const position::Move& move);

    private:
        static void generateSlidingMoves(const position::Position& pos, int startSquare, int piece);
        static void generatePawnMoves(const position::Position& pos);
        static void generateKnightMoves(const position::Position& pos);
        static void generateKingMoves(const position::Position& pos);
        static void generateCastlingMoves(const position::Position& pos);

        static int findKingSquare(const position::Position& pos, int color);
        static bool isSquareAttacked(const position::Position& pos, int square, int attackerColor);

        static position::Move moveBuffer[256];
        static int moveCount;

        static int friendlyColor;
        static int oppositeColor;

    };
}
