#include "Search.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include "../MoveGeneration/GenerateMoves.h"
#include "../Position/Piece.h"

namespace search {

    constexpr int INF = 1'000'000;
    constexpr int MATE_SCORE = 100'000;

    constexpr std::array<int, 7> PIECE_VALUES = {
        0,   // NONE
        100, // PAWN
        320, // KNIGHT
        330, // BISHOP
        500, // ROOK
        900, // QUEEN
        0    // KING
    };

    int evaluate(const position::Position& pos) {
        int score = 0;

        for (int type = position::Piece::PAWN; type <= position::Piece::KING; ++type) {

            position::Bitboard whitePieces = pos.byType[type] & pos.byColor[position::WHITE_INDEX];
            position::Bitboard blackPieces = pos.byType[type] & pos.byColor[position::BLACK_INDEX];

            int whiteCount = 0;
            int blackCount = 0;

            while (whitePieces) {
                position::popLsbIndex(whitePieces);
                ++whiteCount;
            }

            while (blackPieces) {
                position::popLsbIndex(blackPieces);
                ++blackCount;
            }

            score += PIECE_VALUES[type] * (whiteCount - blackCount);
        }

        return pos.turn() ? score : -score;
    }

    int negamax(position::Position& pos, int depth, int alpha, int beta, int ply, std::uint64_t& nodes) {
        ++nodes;

        position::Move moves[256];

        int count = moveGeneration::MoveGenerator::generateLegalMoves(pos, moves);

        // No legal moves means checkmate or stalemate.
        if (count == 0) {
            int color = pos.turn() ? position::Piece::WHITE : position::Piece::BLACK;
            int kingIndex = pos.turn() ? position::WHITE_INDEX : position::BLACK_INDEX;
            int kingSquare = pos.kingSquare(kingIndex);
            bool inCheck = moveGeneration::MoveGenerator::isSquareAttacked(pos, kingSquare, color == position::Piece::WHITE ? position::Piece::BLACK : position::Piece::WHITE);

            if (inCheck)
                return -MATE_SCORE + ply;

            return 0; // Stalemate
        }

        if (depth == 0)
            return evaluate(pos);

        // Simple move ordering: captures first.
        std::stable_sort(moves, moves + count,
            [](const position::Move& a, const position::Move& b) {
                return a.capture() > b.capture();
            }
        );

        int bestScore = -INF;

        for (int i = 0; i < count; ++i) {
            const position::Move& move = moves[i];

            moveGeneration::UndoInfo undo;
            moveGeneration::MoveGenerator::makeMove(pos, move, undo);

            int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes);

            moveGeneration::MoveGenerator::unmakeMove(pos, move, undo);

            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, score);

            // Alpha-beta cutoff.
            if (alpha >= beta)
                break;
        }

        return bestScore;
    }

    SearchResult findBestMove(position::Position& pos, int depth) {
        SearchResult result;
        result.score = -INF;
        result.nodes = 0;

        position::Move moves[256];

        int count = moveGeneration::MoveGenerator::generateLegalMoves(pos, moves);

        if (count == 0 || depth <= 0) {
            result.score = 0;
            return result;
        }

        std::stable_sort(moves, moves + count,
            [](const position::Move& a, const position::Move& b) {
                return a.capture() > b.capture();
            }
        );

        int alpha = -INF;
        const int beta = INF;

        for (int i = 0; i < count; ++i) {
            const position::Move& move = moves[i];

            moveGeneration::UndoInfo undo;
            moveGeneration::MoveGenerator::makeMove(pos, move, undo);

            int score = -negamax(pos, depth - 1, -beta, -alpha, 1, result.nodes);

            moveGeneration::MoveGenerator::unmakeMove(pos, move, undo);

            if (score > result.score) {
                result.score = score;
                result.bestMove = move;
            }

            alpha = std::max(alpha, score);
        }

        return result;
    }
}
