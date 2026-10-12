#pragma once

#include "../Position/Position.h"
#include "../Position/Move.h"

namespace search {
    struct SearchResult {
        position::Move bestMove{};
        int score = 0;
        std::uint64_t nodes = 0;
    };

    SearchResult findBestMove(position::Position& pos, int depth);
}
