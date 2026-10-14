#pragma once

#include "../Position/Position.h"
#include "../Position/Move.h"

namespace search {
    enum class Bound : uint8_t {
        EXACT,
        LOWER,
        UPPER
    };

    struct SearchResult {
        position::Move bestMove{};
        int score = 0;
        std::uint64_t nodes = 0;
    };

    struct TTEntry {
        uint64_t key = 0;
        position::Move bestMove{};
        int score = 0;
        int depth = -1;
        Bound bound = Bound::EXACT;
        bool valid = false;
    };

    SearchResult findBestMove(position::Position& pos, int depth);
}
