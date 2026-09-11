#pragma once
#include <cstdint>
#include "../Position/Bitboard.h"

namespace moveGeneration {
    struct Magic {
        position::Bitboard mask;
        std::uint64_t magic;
        int shift;
        position::Bitboard* attacks;
    };

    class Magics {
    public:
        static void init();
        static position::Bitboard rookAttacks(int square, position::Bitboard occupancy);
        static position::Bitboard bishopAttacks(int square, position::Bitboard occupancy);
        static position::Bitboard queenAttacks(int square, position::Bitboard occupancy);

    private:
        static Magic rookMagics[64];
        static Magic bishopMagics[64];
        static position::Bitboard rookTable[64][4096];
        static position::Bitboard bishopTable[64][512];

        static position::Bitboard computeSlidingAttacks(int square, position::Bitboard blockers, bool isRook);
        static position::Bitboard computeRelevantMask(int square, bool isRook);
        static void initMagicsFor(bool isRook);
        static void verify() noexcept;
    };

    inline void ensureMagicsInit() {
        static const bool initialized = (Magics::init(), true);
        (void)initialized;
    }
}