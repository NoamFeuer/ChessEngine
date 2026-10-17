#include "Search.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>

#include "../MoveGeneration/GenerateMoves.h"
#include "../Position/Piece.h"

namespace search {

    std::atomic<bool> stopSearch{false};
    std::chrono::steady_clock::time_point deadline;
    bool useDeadline = false;

    inline void checkTime(std::uint64_t nodes) {
        if (useDeadline && (nodes & 2047) == 0 && std::chrono::steady_clock::now() >= deadline)
            stopSearch = true;
    }

    constexpr int INF = 1'000'000;
    constexpr int MATE_SCORE = 100'000;
    constexpr size_t TT_SIZE = 1 << 20;

    std::vector<TTEntry> tt(TT_SIZE);

    constexpr std::array<int, 7> PIECE_VALUES = {
        0,    // NONE
        100,  // PAWN
        320,  // KNIGHT
        330,  // BISHOP
        500,  // ROOK
        900,  // QUEEN
        0     // KING
    };

    TTEntry* probeTT(uint64_t key) {
        TTEntry& entry = tt[key & (TT_SIZE - 1)];

        if (entry.valid && entry.key == key)
            return &entry;

        return nullptr;
    }

    void storeTT(uint64_t key, position::Move bestMove, int score, int depth, Bound bound) {
        TTEntry& entry = tt[key & (TT_SIZE - 1)];

        // Replace empty entries, matching positions, or entries that are no deeper than this search.
        if (!entry.valid || entry.key == key || depth >= entry.depth) {
            entry.key = key;
            entry.bestMove = bestMove;
            entry.score = score;
            entry.depth = depth;
            entry.bound = bound;
            entry.valid = true;
        }
    }

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

    int pieceAt(const position::Position& pos, int square) {
        position::Bitboard mask = position::bitBoardOf(square);

        for (int type = position::Piece::PAWN; type <= position::Piece::KING; ++type) {
            if (pos.byType[type] & mask)
                return type;
        }

        return position::Piece::NONE;
    }

    int scoreMove(const position::Position& pos, const position::Move& move) {
        if (!move.capture())
            return 0;

        int victim = move.enPassant() ? position::Piece::PAWN : pieceAt(pos, move.toSquare());
        int attacker = pieceAt(pos, move.fromSquare());

        return 10 * PIECE_VALUES[victim] - PIECE_VALUES[attacker];
    }

    int scoreToTT(int score, int ply) {
        if (score > MATE_SCORE - 1000)
            return score + ply;
        if (score < -MATE_SCORE + 1000)
            return score - ply;
        return score;
    }

    int scoreFromTT(int score, int ply) {
        if (score > MATE_SCORE - 1000)
            return score - ply;
        if (score < -MATE_SCORE + 1000)
            return score + ply;
        return score;
    }

    int negamax(position::Position& pos, int depth, int alpha, int beta, int ply, std::uint64_t& nodes) {
        ++nodes;

        checkTime(nodes);
        if (stopSearch) return 0;

        position::Move moves[256];
        int count = moveGeneration::MoveGenerator::generateLegalMoves(pos, moves);

        if (count == 0) {
            int color = pos.turn() ? position::Piece::WHITE : position::Piece::BLACK;
            int kingIndex = pos.turn() ? position::WHITE_INDEX : position::BLACK_INDEX;
            int kingSquare = pos.kingSquare(kingIndex);
            bool inCheck = moveGeneration::MoveGenerator::isSquareAttacked(pos, kingSquare, color == position::Piece::WHITE ? position::Piece::BLACK : position::Piece::WHITE);

            return inCheck ? -MATE_SCORE + ply : 0;
        }

        if (depth <= 0)
            return evaluate(pos);

        const std::uint64_t key = pos.zorbistHash();
        const int alphaOriginal = alpha;
        const int betaOriginal = beta;

        position::Move ttMove{};
        bool hasTTMove = false;

        if (TTEntry* entry = probeTT(key)) {
            ttMove = entry->bestMove;
            hasTTMove = true;

            if (entry->depth >= depth) {
                int score = scoreFromTT(entry->score, ply);

                if (entry->bound == Bound::EXACT ||
                    (entry->bound == Bound::LOWER && score >= beta) ||
                    (entry->bound == Bound::UPPER && score <= alpha))
                    return score;
            }
        }

        std::stable_sort(moves, moves + count, [&](const position::Move& a, const position::Move& b) {
            return scoreMove(pos, a) > scoreMove(pos, b);
        });

        if (hasTTMove) {
            for (int i = 0; i < count; ++i) {
                if (moves[i] == ttMove) {
                    std::swap(moves[0], moves[i]);
                    break;
                }
            }
        }

        int bestScore = -INF;
        position::Move bestMove = moves[0];

        for (int i = 0; i < count; ++i) {
            const position::Move move = moves[i];

            moveGeneration::UndoInfo undo;
            moveGeneration::MoveGenerator::makeMove(pos, move, undo);

            int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes);

            moveGeneration::MoveGenerator::unmakeMove(pos, move, undo);

            if (stopSearch) return 0;

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }

            alpha = std::max(alpha, score);

            if (alpha >= beta)
                break;
        }

        Bound bound = Bound::EXACT;
        if (bestScore <= alphaOriginal)
            bound = Bound::UPPER;
        else if (bestScore >= betaOriginal)
            bound = Bound::LOWER;

        storeTT(key, bestMove, scoreToTT(bestScore, ply), depth, bound);

        return bestScore;
    }

    // Searches one depth. Returns false if it was aborted (result is then unusable).
    static bool searchRoot(position::Position& pos, int depth, SearchResult& result, std::uint64_t& nodes) {
        position::Move moves[256];
        int count = moveGeneration::MoveGenerator::generateLegalMoves(pos, moves);

        if (count == 0) {
            int color = pos.turn() ? position::Piece::WHITE : position::Piece::BLACK;
            int kingIndex = pos.turn() ? position::WHITE_INDEX : position::BLACK_INDEX;
            int kingSquare = pos.kingSquare(kingIndex);
            bool inCheck = moveGeneration::MoveGenerator::isSquareAttacked(
                pos, kingSquare,
                color == position::Piece::WHITE ? position::Piece::BLACK : position::Piece::WHITE);
            result.score = inCheck ? -MATE_SCORE : 0;
            return true;
        }

        const std::uint64_t key = pos.zorbistHash();
        position::Move ttMove{};
        bool hasTTMove = false;

        if (TTEntry* entry = probeTT(key)) {
            ttMove = entry->bestMove;
            hasTTMove = true;
        }

        std::stable_sort(moves, moves + count, [&](const position::Move& a, const position::Move& b) {
            return scoreMove(pos, a) > scoreMove(pos, b);
        });

        if (hasTTMove) {
            for (int i = 0; i < count; ++i) {
                if (moves[i] == ttMove) {
                    std::swap(moves[0], moves[i]);
                    break;
                }
            }
        }

        SearchResult local;
        local.score = -INF;
        local.bestMove = moves[0];

        int alpha = -INF;
        const int beta = INF;

        for (int i = 0; i < count; ++i) {
            const position::Move move = moves[i];

            moveGeneration::UndoInfo undo;
            moveGeneration::MoveGenerator::makeMove(pos, move, undo);

            int score = -negamax(pos, depth - 1, -beta, -alpha, 1, nodes);

            moveGeneration::MoveGenerator::unmakeMove(pos, move, undo);

            if (stopSearch)
                return false;

            if (score > local.score) {
                local.score = score;
                local.bestMove = move;
            }

            alpha = std::max(alpha, score);
        }

        storeTT(key, local.bestMove, scoreToTT(local.score, 0), depth, Bound::EXACT);

        result.score = local.score;
        result.bestMove = local.bestMove;
        return true;
    }

    SearchResult findBestMove(position::Position& pos, int maxDepth, int timeLimitMs) {
        SearchResult best;
        best.score = -INF;
        best.nodes = 0;

        if (maxDepth <= 0) {
            best.score = evaluate(pos);
            return best;
        }

        stopSearch = false;
        useDeadline = timeLimitMs > 0;
        if (useDeadline)
            deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeLimitMs);

        std::uint64_t nodes = 0;

        for (int depth = 1; depth <= maxDepth; ++depth) {
            SearchResult current;
            if (!searchRoot(pos, depth, current, nodes))
                break;                      // aborted: keep the last completed iteration

            best = current;

            std::cout << "depth " << depth << " score " << best.score << " nodes " << nodes << '\n';

            // Stop early if we've found a forced mate, deeper search won't improve it.
            if (std::abs(best.score) > MATE_SCORE - 1000)
                break;
        }

        best.nodes = nodes;
        return best;
    }
}
