#pragma once
#include <cstdint>

namespace position {
    using Bitboard = std::uint64_t;

    inline Bitboard bitBoardOf(int square) { return 1ull << square; }

    inline int lsb(Bitboard bb) { return __builtin_ctzll(bb); }
    inline int msb(Bitboard bb) { return 63 - __builtin_clzll(bb); }

    inline void popLsb(Bitboard& bb) { bb &= bb - 1; }
    inline int popLsbIndex(Bitboard& bb) {
        int idx = lsb(bb);
        bb &= bb - 1;
        return idx;
    }

    inline int popcount(Bitboard bb) { return __builtin_popcountll(bb); }

    inline int squareOf(int rank, int file) { return rank * 8 + file; }
    inline int rankOf(int square) { return square >> 3; }
    inline int fileOf(int square) { return square & 7; }

    constexpr int WHITE_INDEX = 0;
    constexpr int BLACK_INDEX = 1;
}