#pragma once
#include <cstdint>
#include "../Position/Bitboard.h"

namespace moveGeneration {
    class AttackTables {
    public:
        static void init();

        static position::Bitboard knightAttacks[64];
        static position::Bitboard kingAttacks[64];
        static position::Bitboard pawnAttacks[2][64]; // [0] = white, [1] = black

    private:
        static void initKnightAttacks();
        static void initKingAttacks();
        static void initPawnAttacks();
        static bool verify() noexcept;
    };

    inline void ensureAttackTablesInit() {
        static const bool initialized = (AttackTables::init(), true);
        (void)initialized;
    }
}