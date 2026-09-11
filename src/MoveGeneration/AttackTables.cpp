#include "AttackTables.h"

#include <cassert>

namespace moveGeneration {
    position::Bitboard AttackTables::knightAttacks[64]{};
    position::Bitboard AttackTables::kingAttacks[64]{};
    position::Bitboard AttackTables::pawnAttacks[2][64]{};

    void AttackTables::init() {
        initKnightAttacks();
        initKingAttacks();
        initPawnAttacks();
        assert(verify());
    }

    static bool isOnBoard(int rank, int file) {
        return rank >= 0 && rank < 8 && file >= 0 && file < 8;
    }

    void AttackTables::initKnightAttacks() {
        static const int offsets[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };

        for (int sq = 0; sq < 64; sq++) {
            int r = position::rankOf(sq);
            int f = position::fileOf(sq);

            position::Bitboard bb = 0;
            for (auto& [dr, df] : offsets) {
                int nr = r + dr;
                int nf = f + df;
                if (isOnBoard(nr, nf))
                    bb |= position::bitBoardOf(position::squareOf(nr, nf));
            }
            knightAttacks[sq] = bb;
        }
    }

    void AttackTables::initKingAttacks() {
        static const int offsets[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };

        for (int sq = 0; sq < 64; sq++) {
            int r = position::rankOf(sq);
            int f = position::fileOf(sq);

            position::Bitboard bb = 0;
            for (auto& [dr, df] : offsets) {
                int nr = r + dr;
                int nf = f + df;
                if (isOnBoard(nr, nf))
                    bb |= position::bitBoardOf(position::squareOf(nr, nf));
            }
            kingAttacks[sq] = bb;
        }
    }

    void AttackTables::initPawnAttacks() {
        for (int sq = 0; sq < 64; sq++) {
            int r = position::rankOf(sq);
            int f = position::fileOf(sq);

            position::Bitboard whiteBB = 0;
            if (r + 1 < 8) {
                if (f - 1 >= 0) whiteBB |= position::bitBoardOf(position::squareOf(r + 1, f - 1));
                if (f + 1 < 8) whiteBB |= position::bitBoardOf(position::squareOf(r + 1, f + 1));
            }
            pawnAttacks[0][sq] = whiteBB;

            position::Bitboard blackBB = 0;
            if (r - 1 >= 0) {
                if (f - 1 >= 0) blackBB |= position::bitBoardOf(position::squareOf(r - 1, f - 1));
                if (f + 1 < 8) blackBB |= position::bitBoardOf(position::squareOf(r - 1, f + 1));
            }
            pawnAttacks[1][sq] = blackBB;
        }
    }

    bool AttackTables::verify() noexcept {
        for (int sq = 0; sq < 64; sq++) {
            int r = position::rankOf(sq);
            int f = position::fileOf(sq);

            auto checkKnight = [&](int dr, int df, position::Bitboard expected) {
                position::Bitboard got = knightAttacks[sq];
                return ((expected & got) == expected);
            };

            // Knight checks
            position::Bitboard expectedKnight = 0;
            static const int nd[8][2] = {
                {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                {1, -2}, {1, 2}, {2, -1}, {2, 1}
            };
            for (auto& [dr, df] : nd) {
                if (isOnBoard(r + dr, f + df))
                    expectedKnight |= position::bitBoardOf(position::squareOf(r + dr, f + df));
            }
            if (knightAttacks[sq] != expectedKnight) return false;

            // King checks
            position::Bitboard expectedKing = 0;
            static const int kd[8][2] = {
                {-1, -1}, {-1, 0}, {-1, 1},
                {0, -1},           {0, 1},
                {1, -1},  {1, 0},  {1, 1}
            };
            for (auto& [dr, df] : kd) {
                if (isOnBoard(r + dr, f + df))
                    expectedKing |= position::bitBoardOf(position::squareOf(r + dr, f + df));
            }
            if (kingAttacks[sq] != expectedKing) return false;

            // White pawn attacks
            position::Bitboard expectedWhite = 0;
            if (r + 1 < 8) {
                if (f - 1 >= 0) expectedWhite |= position::bitBoardOf(position::squareOf(r + 1, f - 1));
                if (f + 1 < 8) expectedWhite |= position::bitBoardOf(position::squareOf(r + 1, f + 1));
            }
            if (pawnAttacks[0][sq] != expectedWhite) return false;

            // Black pawn attacks
            position::Bitboard expectedBlack = 0;
            if (r - 1 >= 0) {
                if (f - 1 >= 0) expectedBlack |= position::bitBoardOf(position::squareOf(r - 1, f - 1));
                if (f + 1 < 8) expectedBlack |= position::bitBoardOf(position::squareOf(r - 1, f + 1));
            }
            if (pawnAttacks[1][sq] != expectedBlack) return false;
        }
        return true;
    }
}