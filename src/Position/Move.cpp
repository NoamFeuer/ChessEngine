#include "Move.h"

namespace position {
    Move::Move(int fromSquare, int toSquare, bool castle, bool enPassant, int promotion) {
        this->fromSquare = fromSquare;
        this->toSquare = toSquare;
        this->capture = false;
        this->castle = castle;
        this->enPassant = enPassant;
        this->promotion = promotion;
    }
}
