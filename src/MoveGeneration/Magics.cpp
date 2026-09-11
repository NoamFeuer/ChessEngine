#include "Magics.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

namespace moveGeneration {
    Magic Magics::rookMagics[64]{};
    Magic Magics::bishopMagics[64]{};
    position::Bitboard Magics::rookTable[64][4096]{};
    position::Bitboard Magics::bishopTable[64][512]{};

    static bool isOnBoard(int rank, int file) {
        return rank >= 0 && rank < 8 && file >= 0 && file < 8;
    }

    static std::uint64_t splitmix64(std::uint64_t& state) {
        std::uint64_t z = state += 0x9E3779B97F4A7C15ULL;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    position::Bitboard Magics::computeRelevantMask(int square, bool isRook) {
        position::Bitboard mask = 0;
        int r = position::rankOf(square);
        int f = position::fileOf(square);

        static const int rookDirs[4][2]   = {{0,1},{0,-1},{1,0},{-1,0}};
        static const int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
        const auto (*dirs)[2] = isRook ? rookDirs : bishopDirs;

        for (int d = 0; d < 4; d++) {
            int nr = r + dirs[d][0];
            int nf = f + dirs[d][1];

            while (isOnBoard(nr, nf)) {
                int nnr = nr + dirs[d][0];
                int nnf = nf + dirs[d][1];
                bool atEdge = !isOnBoard(nnr, nnf);

                if (!atEdge)
                    mask |= position::bitBoardOf(position::squareOf(nr, nf));

                nr = nnr;
                nf = nnf;
            }
        }
        return mask;
    }

    position::Bitboard Magics::computeSlidingAttacks(int square, position::Bitboard blockers, bool isRook) {
        position::Bitboard attacks = 0;
        int r = position::rankOf(square);
        int f = position::fileOf(square);

        static const int rookDirs[4][2]   = {{0,1},{0,-1},{1,0},{-1,0}};
        static const int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
        const auto (*dirs)[2] = isRook ? rookDirs : bishopDirs;

        for (int d = 0; d < 4; d++) {
            int nr = r + dirs[d][0];
            int nf = f + dirs[d][1];

            while (isOnBoard(nr, nf)) {
                int sq = position::squareOf(nr, nf);
                position::Bitboard bit = position::bitBoardOf(sq);
                attacks |= bit;
                if (blockers & bit) break;

                nr += dirs[d][0];
                nf += dirs[d][1];
            }
        }
        return attacks;
    }

    void Magics::init() {
        initMagicsFor(true);
        initMagicsFor(false);
        verify();
    }

    void Magics::initMagicsFor(bool isRook) {
        std::uint64_t seed = isRook ? 0x9E3779B97F4A7C15ULL : 0xA4CA8D40B53F7C83ULL;

        for (int sq = 0; sq < 64; sq++) {
            position::Bitboard mask = computeRelevantMask(sq, isRook);
            int bits = position::popcount(mask);
            int shift = 64 - bits;

            Magic& m = isRook ? rookMagics[sq] : bishopMagics[sq];
            m.mask = mask;
            m.shift = shift;

            position::Bitboard* table = isRook ? rookTable[sq] : bishopTable[sq];
            m.attacks = table;
            m.magic = 0;

            for (int attempt = 0; attempt < 10000000; attempt++) {
                std::uint64_t r = splitmix64(seed);
                // Magics need to be sparse; AND together a few randoms.
                std::uint64_t magic = r & splitmix64(seed) & splitmix64(seed);

                // Heuristic: the top bits of the product must be well distributed.
                if (position::popcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

                std::uint64_t used[4096];
                std::memset(used, 0, sizeof(std::uint64_t) * (1u << bits));

                bool collision = false;
                position::Bitboard occ = 0;

                do {
                    position::Bitboard attack = computeSlidingAttacks(sq, occ, isRook);
                    int idx = static_cast<int>((occ * magic) >> shift);
                    std::uint64_t val = attack + 1;  // attack is never 0

                    if (used[idx] != 0 && used[idx] != val) {
                        collision = true;
                        break;
                    }
                    used[idx] = val;
                    occ = (occ - mask) & mask;
                } while (occ != 0);

                if (collision) continue;

                // Magic works — build table
                std::memset(table, 0, sizeof(position::Bitboard) * (1u << bits));
                occ = 0;
                do {
                    position::Bitboard attack = computeSlidingAttacks(sq, occ, isRook);
                    int idx = static_cast<int>((occ * magic) >> shift);
                    table[idx] = attack;
                    occ = (occ - mask) & mask;
                } while (occ != 0);

                m.magic = magic;
                break;
            }

            if (m.magic == 0) {
                throw std::runtime_error("Failed to find magic for square");
            }
        }
    }

    void Magics::verify() noexcept {
        // For each square, verify all subsets map to correct attacks
        for (int sq = 0; sq < 64; sq++) {
            for (bool isRook : {true, false}) {
                const Magic& m = isRook ? rookMagics[sq] : bishopMagics[sq];
                position::Bitboard occ = 0;
                do {
                    position::Bitboard expected = computeSlidingAttacks(sq, occ, isRook);
                    position::Bitboard actual = m.attacks[((occ & m.mask) * m.magic) >> m.shift];
                    assert(actual == expected);
                    occ = (occ - m.mask) & m.mask;
                } while (occ != 0);
            }
        }
    }

    position::Bitboard Magics::rookAttacks(int square, position::Bitboard occupancy) {
        ensureMagicsInit();
        const Magic& m = rookMagics[square];
        return m.attacks[((occupancy & m.mask) * m.magic) >> m.shift];
    }

    position::Bitboard Magics::bishopAttacks(int square, position::Bitboard occupancy) {
        ensureMagicsInit();
        const Magic& m = bishopMagics[square];
        return m.attacks[((occupancy & m.mask) * m.magic) >> m.shift];
    }

    position::Bitboard Magics::queenAttacks(int square, position::Bitboard occupancy) {
        return rookAttacks(square, occupancy) | bishopAttacks(square, occupancy);
    }
}