#include "GenerateMoves.h"

#include <cstdlib>
#include <cstring>
#include <cassert>

#include "AttackTables.h"
#include "Magics.h"
#include "../Position/Piece.h"

namespace moveGeneration {
    static_assert(sizeof(position::Move) == 4, "Move must be packed into 4 bytes");

    position::Move MoveGenerator::moveBuffer[256]{};
    int MoveGenerator::moveCount{};
    int MoveGenerator::friendlyColor{};
    int MoveGenerator::oppositeColor{};
    position::Bitboard MoveGenerator::friendlyBB{};
    position::Bitboard MoveGenerator::oppositeBB{};

    static position::Bitboard rankMaskBB(int rank) { return 0xFFull << (rank * 8); }
    static position::Bitboard fileMaskBB(int file) { return 0x0101010101010101ull << file; }
    static const position::Bitboard FILE_A_MASK = 0x0101010101010101ull;
    static const position::Bitboard FILE_H_MASK = 0x8080808080808080ull;

    static int colorIndex(int color) {
        return color == position::Piece::WHITE ? position::WHITE_INDEX : position::BLACK_INDEX;
    }

    static int pieceOn(const position::Position& pos, int sq) {
        position::Bitboard b = position::bitBoardOf(sq);
        int colorMask = (b & pos.byColor[position::WHITE_INDEX]) ? position::Piece::WHITE : position::Piece::BLACK;
        for (int t = position::Piece::PAWN; t <= position::Piece::KING; t++) {
            if (b & pos.byType[t]) return colorMask + t;
        }
        return 0;
    }

    std::vector<position::Move> MoveGenerator::generatePseudoLegalMoves(const position::Position& pos) {
        position::Move moves[256];
        int n = generatePseudoLegalMoves(pos, moves);
        return std::vector<position::Move>(moves, moves + n);
    }

    int MoveGenerator::generatePseudoLegalMoves(const position::Position& pos, position::Move* outMoves) {
        ensureAttackTablesInit();
        moveCount = 0;
        friendlyColor = pos.turn() ? position::Piece::WHITE : position::Piece::BLACK;
        oppositeColor = pos.turn() ? position::Piece::BLACK : position::Piece::WHITE;
        friendlyBB = pos.byColor[colorIndex(friendlyColor)];
        oppositeBB = pos.byColor[colorIndex(oppositeColor)];

        generatePawnMoves(pos);
        generateKnightMoves(pos);
        generateKingMoves(pos);
        generateCastlingMoves(pos);

        for (int i = 0; i < 64; i++) {
            position::Bitboard b = position::bitBoardOf(i);
            if ((b & friendlyBB) == 0) continue;

            for (int t : {position::Piece::BISHOP, position::Piece::ROOK, position::Piece::QUEEN}) {
                if (b & pos.byType[t])
                    generateSlidingMoves(pos, i, t);
            }
        }

        std::memcpy(outMoves, moveBuffer, moveCount * sizeof(position::Move));
        return moveCount;
    }

    std::vector<position::Move> MoveGenerator::generateLegalMoves(const position::Position& pos) {
        position::Move moves[256];
        position::Position work(pos);
        int n = generateLegalMoves(work, moves);
        return std::vector<position::Move>(moves, moves + n);
    }

    int MoveGenerator::generateLegalMoves(position::Position& pos, position::Move* outMoves) {
        position::Move pseudo[256];
        int pseudoCount = generatePseudoLegalMoves(pos, pseudo);

        int friendlyIdx = colorIndex(friendlyColor);
        int oppositeIdx = colorIndex(oppositeColor);

        if (!(pos.byColor[friendlyIdx] & pos.byType[position::Piece::KING])) return 0;
        if (!(pos.byColor[oppositeIdx] & pos.byType[position::Piece::KING])) return 0;

        int legalCount = 0;
        for (int i = 0; i < pseudoCount; i++) {
            const position::Move& mv = pseudo[i];

            if (mv.castle()) {
                int ks = pos.kingSquare(friendlyIdx);
                bool kingSide = (mv.toSquare() % 8) == 6;
                int through = kingSide ? ks + 1 : ks - 1;
                int dest = kingSide ? ks + 2 : ks - 2;

                if (isSquareAttacked(pos, ks, oppositeColor)) continue;
                if (isSquareAttacked(pos, through, oppositeColor)) continue;
                if (isSquareAttacked(pos, dest, oppositeColor)) continue;
            }

            int enemyKing = pos.kingSquare(oppositeIdx);
            if (mv.toSquare() == enemyKing) continue;

            UndoInfo undo;
            makeMove(pos, mv, undo);
            int king = pos.kingSquare(friendlyIdx);
            bool legal = !isSquareAttacked(pos, king, oppositeColor);
            unmakeMove(pos, mv, undo);

            if (legal) outMoves[legalCount++] = mv;
        }

        return legalCount;
    }

    std::uint64_t MoveGenerator::perft(position::Position& pos, int depth) {
        if (depth <= 1) {
            position::Move moves[256];
            return generateLegalMoves(pos, moves);
        }

        position::Move moves[256];
        int count = generateLegalMoves(pos, moves);

        std::uint64_t nodes = 0;
        for (int i = 0; i < count; i++) {
            UndoInfo undo;
            makeMove(pos, moves[i], undo);
            nodes += perft(pos, depth - 1);
            unmakeMove(pos, moves[i], undo);
        }

        return nodes;
    }

    void MoveGenerator::makeMove(position::Position& pos, const position::Move& move, UndoInfo& undo) {
        int from = move.fromSquare();
        int to = move.toSquare();
        position::Bitboard fromBBb = position::bitBoardOf(from);
        position::Bitboard toBBb = position::bitBoardOf(to);

        undo.movedPiece = pieceOn(pos, from);
        undo.capturedPiece = (toBBb & (pos.byColor[0] | pos.byColor[1])) ? pieceOn(pos, to) : 0;
        undo.enPassantSquare = pos.enPassantSquare;
        undo.castlingRights = pos.castlingRights;

        int color = position::Piece::getPieceColor(undo.movedPiece);
        int type = position::Piece::getPieceType(undo.movedPiece);
        int colorIdx = colorIndex(color);

        if (move.capture()) {
            if (move.enPassant()) {
                int capturedSq = to + (color == position::Piece::WHITE ? -8 : +8);
                position::Bitboard cBBb = position::bitBoardOf(capturedSq);
                undo.capturedPiece = pieceOn(pos, capturedSq);
                pos.byColor[1 - colorIdx] ^= cBBb;
                pos.byType[position::Piece::PAWN] ^= cBBb;
            } else {
                int cType = position::Piece::getPieceType(undo.capturedPiece);
                pos.byColor[1 - colorIdx] ^= toBBb;
                pos.byType[cType] ^= toBBb;
            }
        }

        pos.byColor[colorIdx] ^= fromBBb | toBBb;
        pos.byType[type] ^= fromBBb | toBBb;

        if (move.promotion() != position::Piece::NONE) {
            pos.byType[position::Piece::PAWN] ^= toBBb;
            pos.byType[move.promotion()] ^= toBBb;
        }

        if (move.castle()) {
            position::Bitboard rookToggle;
            if (to % 8 == 6) {
                rookToggle = position::bitBoardOf(to - 1) ^ position::bitBoardOf(to + 1);
            } else {
                rookToggle = position::bitBoardOf(to + 1) ^ position::bitBoardOf(to - 2);
            }
            pos.byType[position::Piece::ROOK] ^= rookToggle;
            pos.byColor[colorIdx] ^= rookToggle;
        }

        if (type == position::Piece::PAWN && std::abs(to - from) == 16) {
            pos.enPassantSquare = from + (color == position::Piece::WHITE ? +8 : -8);
        } else {
            pos.enPassantSquare = -1;
        }

        if (type == position::Piece::KING) {
            if (color == position::Piece::WHITE) {
                pos.castlingRights &= ~(position::Position::WHITE_KINGSIDE | position::Position::WHITE_QUEENSIDE);
            } else {
                pos.castlingRights &= ~(position::Position::BLACK_KINGSIDE | position::Position::BLACK_QUEENSIDE);
            }
        }

        if (from == 0 || to == 0)   pos.castlingRights &= ~position::Position::WHITE_QUEENSIDE;
        if (from == 7 || to == 7)   pos.castlingRights &= ~position::Position::WHITE_KINGSIDE;
        if (from == 56 || to == 56) pos.castlingRights &= ~position::Position::BLACK_QUEENSIDE;
        if (from == 63 || to == 63) pos.castlingRights &= ~position::Position::BLACK_KINGSIDE;

        pos.flipTurn();
    }

    void MoveGenerator::unmakeMove(position::Position& pos, const position::Move& move, const UndoInfo& undo) {
        int from = move.fromSquare();
        int to = move.toSquare();
        position::Bitboard fromBBb = position::bitBoardOf(from);
        position::Bitboard toBBb = position::bitBoardOf(to);

        int color = position::Piece::getPieceColor(undo.movedPiece);
        int type = position::Piece::getPieceType(undo.movedPiece);
        int colorIdx = colorIndex(color);

        // Undo promotion first so byType[PAWN] is fully restored before
        // the captured piece with the same type is put back on `to`.
        if (move.promotion() != position::Piece::NONE) {
            pos.byType[position::Piece::PAWN] ^= toBBb;
            pos.byType[move.promotion()] ^= toBBb;
        }

        pos.byColor[colorIdx] ^= fromBBb | toBBb;
        pos.byType[type] ^= fromBBb | toBBb;

        if (move.castle()) {
            position::Bitboard rookToggle;
            if (to % 8 == 6) {
                rookToggle = position::bitBoardOf(to - 1) ^ position::bitBoardOf(to + 1);
            } else {
                rookToggle = position::bitBoardOf(to + 1) ^ position::bitBoardOf(to - 2);
            }
            pos.byType[position::Piece::ROOK] ^= rookToggle;
            pos.byColor[colorIdx] ^= rookToggle;
        }

        if (move.enPassant()) {
            int capturedSq = to + (color == position::Piece::WHITE ? -8 : +8);
            position::Bitboard cBBb = position::bitBoardOf(capturedSq);
            pos.byColor[1 - colorIdx] |= cBBb;
            pos.byType[position::Piece::PAWN] |= cBBb;
        } else if (undo.capturedPiece) {
            int cType = position::Piece::getPieceType(undo.capturedPiece);
            pos.byColor[1 - colorIdx] |= toBBb;
            pos.byType[cType] |= toBBb;
        }

        pos.enPassantSquare = undo.enPassantSquare;
        pos.castlingRights = undo.castlingRights;
        pos.flipTurn();
    }

    void MoveGenerator::applyMove(position::Position& pos, const position::Move& move) {
        UndoInfo discard;
        makeMove(pos, move, discard);
    }

    void MoveGenerator::generateSlidingMoves(const position::Position& pos, int startSquare, int piece) {
        position::Bitboard occ = pos.byColor[0] | pos.byColor[1];
        position::Bitboard targets;

        if (piece == position::Piece::BISHOP)       targets = Magics::bishopAttacks(startSquare, occ);
        else if (piece == position::Piece::ROOK)    targets = Magics::rookAttacks(startSquare, occ);
        else                                        targets = Magics::queenAttacks(startSquare, occ);

        targets &= ~friendlyBB;
        emitMovesFromBitboard(targets, startSquare);
    }

    void MoveGenerator::addPromoMoves(int from, int to, bool capture) {
        for (int promo : {position::Piece::QUEEN, position::Piece::ROOK,
                          position::Piece::BISHOP, position::Piece::KNIGHT}) {
            position::Move& m = moveBuffer[moveCount++];
            m = position::Move(from, to, false, false, promo);
            if (capture) m.setCapture();
        }
    }

    void MoveGenerator::generatePawnMoves(const position::Position& pos) {
        position::Bitboard pawns = friendlyBB & pos.byType[position::Piece::PAWN];
        if (!pawns) return;

        bool white = friendlyColor == position::Piece::WHITE;
        position::Bitboard occ = pos.byColor[0] | pos.byColor[1];
        position::Bitboard empty = ~occ;
        position::Bitboard promoRank = white ? rankMaskBB(7) : rankMaskBB(0);
        position::Bitboard startRank = white ? rankMaskBB(1) : rankMaskBB(6);

        int forward = white ? 8 : -8;

        // Single pushes
        position::Bitboard singlePush = white ? (pawns << 8) : (pawns >> 8);
        singlePush &= empty;

        // Promotion pushes
        position::Bitboard promoPush = singlePush & promoRank;
        while (promoPush) {
            int to = position::popLsbIndex(promoPush);
            addPromoMoves(to - forward, to, false);
        }

        // Quiet pushes
        position::Bitboard quietPush = singlePush & ~promoRank;
        while (quietPush) {
            int to = position::popLsbIndex(quietPush);
            moveBuffer[moveCount++] = position::Move(to - forward, to);
        }

        // Double pushes (must also verify the intermediate square is empty)
        position::Bitboard doublePush = white ? ((pawns & startRank) << 16) : ((pawns & startRank) >> 16);
        doublePush &= empty & (white ? (singlePush << 8) : (singlePush >> 8));
        while (doublePush) {
            int to = position::popLsbIndex(doublePush);
            moveBuffer[moveCount++] = position::Move(to - 2 * forward, to);
        }

        // En passant
        if (pos.enPassantSquare != -1) {
            position::Bitboard epBack = position::bitBoardOf(pos.enPassantSquare - forward);
            if (oppositeBB & pos.byType[position::Piece::PAWN] & epBack) {
                position::Bitboard epBit = position::bitBoardOf(pos.enPassantSquare);
                position::Bitboard movers = pawns;
                int ci = colorIndex(friendlyColor);
                while (movers) {
                    int sq = position::popLsbIndex(movers);
                    if (AttackTables::pawnAttacks[ci][sq] & epBit) {
                        position::Move& m = moveBuffer[moveCount++];
                        m = position::Move(sq, pos.enPassantSquare, false, true);
                        m.setCapture();
                    }
                }
            }
        }

        // Captures
        position::Bitboard capPawns = pawns;
        int ci = colorIndex(friendlyColor);
        while (capPawns) {
            int sq = position::popLsbIndex(capPawns);

            position::Bitboard caps = AttackTables::pawnAttacks[ci][sq] & oppositeBB;
            position::Bitboard promoCap = caps & promoRank;
            position::Bitboard quietCap = caps & ~promoRank;

            while (promoCap) {
                int to = position::popLsbIndex(promoCap);
                addPromoMoves(sq, to, true);
            }

            while (quietCap) {
                int to = position::popLsbIndex(quietCap);
                position::Move& m = moveBuffer[moveCount++];
                m = position::Move(sq, to);
                m.setCapture();
            }
        }
    }

    void MoveGenerator::generateKnightMoves(const position::Position& pos) {
        position::Bitboard knights = friendlyBB & pos.byType[position::Piece::KNIGHT];
        while (knights) {
            int sq = position::popLsbIndex(knights);
            position::Bitboard targets = AttackTables::knightAttacks[sq] & ~friendlyBB;
            emitMovesFromBitboard(targets, sq);
        }
    }

    void MoveGenerator::generateKingMoves(const position::Position& pos) {
        position::Bitboard kings = friendlyBB & pos.byType[position::Piece::KING];
        if (!kings) return;
        int sq = position::lsb(kings);
        position::Bitboard targets = AttackTables::kingAttacks[sq] & ~friendlyBB;
        emitMovesFromBitboard(targets, sq);
    }

    void MoveGenerator::emitMovesFromBitboard(position::Bitboard targets, int fromSquare) {
        while (targets) {
            int to = position::popLsbIndex(targets);
            position::Bitboard toBBb = position::bitBoardOf(to);

            position::Move& m = moveBuffer[moveCount++];
            m = position::Move(fromSquare, to);
            if (toBBb & oppositeBB) m.setCapture();
        }
    }

    void MoveGenerator::generateCastlingMoves(const position::Position& pos) {
        bool white = friendlyColor == position::Piece::WHITE;
        position::Bitboard occ = pos.byColor[0] | pos.byColor[1];
        position::Bitboard kingBBb = friendlyBB & pos.byType[position::Piece::KING];
        if (!kingBBb) return;
        int kingSq = position::lsb(kingBBb);

        if (white) {
            if (kingSq != 4) return;

            if ((pos.castlingRights & position::Position::WHITE_KINGSIDE) &&
                (occ & (position::bitBoardOf(5) | position::bitBoardOf(6))) == 0 &&
                (friendlyBB & position::bitBoardOf(7))) {
                moveBuffer[moveCount++] = position::Move(4, 6, true);
            }

            if ((pos.castlingRights & position::Position::WHITE_QUEENSIDE) &&
                (occ & (position::bitBoardOf(1) | position::bitBoardOf(2) | position::bitBoardOf(3))) == 0 &&
                (friendlyBB & position::bitBoardOf(0))) {
                moveBuffer[moveCount++] = position::Move(4, 2, true);
            }
        } else {
            if (kingSq != 60) return;

            if ((pos.castlingRights & position::Position::BLACK_KINGSIDE) &&
                (occ & (position::bitBoardOf(61) | position::bitBoardOf(62))) == 0 &&
                (friendlyBB & position::bitBoardOf(63))) {
                moveBuffer[moveCount++] = position::Move(60, 62, true);
            }

            if ((pos.castlingRights & position::Position::BLACK_QUEENSIDE) &&
                (occ & (position::bitBoardOf(57) | position::bitBoardOf(58) | position::bitBoardOf(59))) == 0 &&
                (friendlyBB & position::bitBoardOf(56))) {
                moveBuffer[moveCount++] = position::Move(60, 58, true);
            }
        }
    }

    bool MoveGenerator::isSquareAttacked(const position::Position& pos, int square, int attackerColor) {
        int attackerIdx = colorIndex(attackerColor);
        position::Bitboard occ = pos.byColor[0] | pos.byColor[1];
        position::Bitboard attBB = pos.byColor[attackerIdx];

        if (AttackTables::pawnAttacks[1 ^ attackerIdx][square] & attBB & pos.byType[position::Piece::PAWN])
            return true;
        if (AttackTables::knightAttacks[square] & attBB & pos.byType[position::Piece::KNIGHT])
            return true;
        if (AttackTables::kingAttacks[square] & attBB & pos.byType[position::Piece::KING])
            return true;

        position::Bitboard rookAndQueen = attBB & (pos.byType[position::Piece::ROOK] | pos.byType[position::Piece::QUEEN]);
        position::Bitboard bishopAndQueen = attBB & (pos.byType[position::Piece::BISHOP] | pos.byType[position::Piece::QUEEN]);

        if (Magics::rookAttacks(square, occ) & rookAndQueen)
            return true;
        if (Magics::bishopAttacks(square, occ) & bishopAndQueen)
            return true;

        return false;
    }
}