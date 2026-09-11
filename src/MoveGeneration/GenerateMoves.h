#pragma once
#include <cstdint>
#include <vector>
#include "../Position/Move.h"
#include "../Position/Position.h"

namespace moveGeneration {
    struct UndoInfo {
        int movedPiece;
        int capturedPiece;
        int enPassantSquare;
        int castlingRights;
    };

    class MoveGenerator {
    public:
        static std::vector<position::Move> generatePseudoLegalMoves(const position::Position& pos);
        static int generatePseudoLegalMoves(const position::Position& pos, position::Move* outMoves);

        static std::vector<position::Move> generateLegalMoves(const position::Position& pos);
        static int generateLegalMoves(position::Position& pos, position::Move* outMoves);

        static std::uint64_t perft(position::Position& pos, int depth);

        static void makeMove(position::Position& pos, const position::Move& move, UndoInfo& undo);
        static void unmakeMove(position::Position& pos, const position::Move& move, const UndoInfo& undo);
        static void applyMove(position::Position& pos, const position::Move& move);

    private:
        static void generatePawnMoves(const position::Position& pos);
        static void generateKnightMoves(const position::Position& pos);
        static void generateKingMoves(const position::Position& pos);
        static void generateSlidingMoves(const position::Position& pos, int startSquare, int piece);
        static void generateCastlingMoves(const position::Position& pos);

        static bool isSquareAttacked(const position::Position& pos, int square, int attackerColor);

        static position::Move moveBuffer[256];
        static int moveCount;

        static int friendlyColor;
        static int oppositeColor;
        static position::Bitboard friendlyBB;
        static position::Bitboard oppositeBB;

        static void emitMovesFromBitboard(position::Bitboard targets, int fromSquare);
        static void addPromoMoves(int from, int to, bool capture);
    };
}