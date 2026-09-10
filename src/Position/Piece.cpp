#include "Piece.h"

namespace position {
    int Piece::getPieceType(int piece) {
        // Zero out the color bits to get only the type
        return piece & 7; // Same thing as doing: piece & 0b00111
    }

    int Piece::getPieceColor(int piece) {
        // Zero out the type bits to get only the color
        return piece & 24; // Same thing as doing: piece & 0b11000
    }

    bool Piece::isType(int piece, int type) {
        return getPieceType(piece) == type;
    }

    bool Piece::isColor(int piece, int type) {
        return getPieceColor(piece) == type;
    }
}
