#include "PMD.h"

#include <algorithm>

namespace moveGeneration {

    std::array<std::array<int, 8>, 64> PMD::makeNumSquaresToEdge() {
        std::array<std::array<int, 8>, 64> result{};

        for (int file = 0; file < 8; file++)
        {
            for (int rank = 0; rank < 8; rank++)
            {
                int numNorth = rank;
                int numSouth = 7 - rank;
                int numWest  = file;
                int numEast  = 7 - file;

                int squareIndex = rank * 8 + file;

                result[squareIndex] =
                {
                    numNorth,
                    numSouth,
                    numWest,
                    numEast,
                    std::min(numNorth, numWest),
                    std::min(numSouth, numEast),
                    std::min(numNorth, numEast),
                    std::min(numSouth, numWest)
                };
            }
        }

        return result;
    }

    std::array<std::array<int, 8>, 64> PMD::numSquaresToEdge = PMD::makeNumSquaresToEdge();

    std::array<int, 8> PMD::directionOffsets = { -8, 8, -1, 1, -9, 9, -7, 7 };

    std::array<int, 8> PMD::knightOffsets = { -17, -15, -10, -6, 6, 10, 15, 17 };
    std::array<int, 8> PMD::knightFileOffsets = { -1, 1, -2, 2, -2, 2, -1, 1 };
    std::array<int, 8> PMD::knightRankOffsets = { -2, -2, -1, -1, 1, 1, 2, 2 };
}