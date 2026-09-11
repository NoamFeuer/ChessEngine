#include <stdexcept>
#include <cctype>
#include <sstream>

#include "Position.h"
#include "Piece.h"

namespace position {
    Position::Position() {
        loadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    Position::Position(const std::string& fen) {
        loadFen(fen);
    }

    int Position::colorToIndex(Bitboard colorMask) {
        return colorMask == 16 ? WHITE_INDEX : BLACK_INDEX;
    }

    int Position::kingSquare(int colorIndex) const {
        return lsb(byColor[colorIndex] & byType[6]);
    }

    bool Position::turn() const {
        return colorToMove == WHITE_INDEX;
    }

    void Position::flipTurn() {
        colorToMove ^= 1;
    }

    void Position::loadFen(const std::string& fen) {
        byColor[0] = 0;
        byColor[1] = 0;
        for (auto& bb : byType) bb = 0;

        std::istringstream stream(fen);
        std::string board;
        std::string sideToMove;
        std::string castle;
        std::string ep;

        stream >> board >> sideToMove >> castle >> ep;

        int rank = 7;
        int file = 0;

        for (char c : board) {
            if (c == '/') {
                rank--;
                file = 0;
                continue;
            }

            if (isdigit(static_cast<unsigned char>(c))) {
                file += c - '0';
                continue;
            }

            int colorIndex = islower(static_cast<unsigned char>(c)) ? BLACK_INDEX : WHITE_INDEX;

            int type;
            switch (tolower(static_cast<unsigned char>(c))) {
                case 'p': type = Piece::PAWN; break;
                case 'n': type = Piece::KNIGHT; break;
                case 'b': type = Piece::BISHOP; break;
                case 'r': type = Piece::ROOK; break;
                case 'q': type = Piece::QUEEN; break;
                case 'k': type = Piece::KING; break;
                default: throw std::invalid_argument("Invalid FEN! Not a valid character");
            }

            byColor[colorIndex] |= bitBoardOf(squareOf(rank, file));
            byType[type] |= bitBoardOf(squareOf(rank, file));
            file++;
        }

        colorToMove = (sideToMove == "w") ? WHITE_INDEX : BLACK_INDEX;

        castlingRights = 0;
        if (castle.find('K') != std::string::npos) castlingRights |= WHITE_KINGSIDE;
        if (castle.find('Q') != std::string::npos) castlingRights |= WHITE_QUEENSIDE;
        if (castle.find('k') != std::string::npos) castlingRights |= BLACK_KINGSIDE;
        if (castle.find('q') != std::string::npos) castlingRights |= BLACK_QUEENSIDE;

        enPassantSquare = (ep == "-") ? -1 : fenSquareToIndex(ep);
    }

    int Position::fenSquareToIndex(const std::string& square) const {
        int file = square[0] - 'a';
        int rank = square[1] - '1';
        return rank * 8 + file;
    }
}