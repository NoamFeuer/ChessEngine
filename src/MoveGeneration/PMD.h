#pragma once

#include <array>

namespace moveGeneration {
    struct PMD {
        static std::array<std::array<int, 8>, 64> numSquaresToEdge;

        static std::array<int, 8> directionOffsets;
        static std::array<int, 8> knightOffsets;
        static std::array<int, 8> knightFileOffsets;
        static std::array<int, 8> knightRankOffsets;

    private:
        static std::array<std::array<int, 8>, 64> makeNumSquaresToEdge();
    };
}

