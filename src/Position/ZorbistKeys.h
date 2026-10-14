#pragma once

#include <array>
#include <cstdint>
#include <random>

namespace position {
    struct ZobristKeys {
        std::array<std::array<uint64_t, 64>, 12> pieces{};

        uint64_t sideToMove = 0;

        std::array<uint64_t, 16> castling{};
        std::array<uint64_t, 8> enPassant{};

        ZobristKeys() {
            std::mt19937_64 rng(1070372);

            for (auto& piece : pieces) {
                for (auto& square : piece) {
                    square = rng();
                }
            }
                
            sideToMove = rng();

            for (auto& key : castling){
                key = rng();
            }
                
            for (auto& key : enPassant) {
                key = rng();
            }
        }
    };

    inline const ZobristKeys zobristKeys{};
}
