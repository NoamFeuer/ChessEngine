#include <stdexcept>
#include <algorithm>
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

    Position::Position(const Position& position) {
        std::copy(position.squares, position.squares + 64, squares);
        colorToMove = position.turn() ? Piece::WHITE : Piece::BLACK;
        castlingRights = position.castlingRights;
        enPassantSquare = position.enPassantSquare;
    }

    Position &Position::operator=(const Position& position) {
        if (this != &position) {
            std::copy(position.squares, position.squares + 64, squares);
            colorToMove = position.turn() ? Piece::WHITE : Piece::BLACK;
            castlingRights = position.castlingRights;
            enPassantSquare = position.enPassantSquare;
        }

        return *this;
    }

    Position::~Position() {

    }

    void Position::loadFen(const std::string &fen) {
        int index = 0;

        std::istringstream stream(fen);
        std::string board;
        std::string sideToMove;
        std::string castle;
        std::string ep;

        stream >> board >> sideToMove >> castle >> ep;

        for (char c : board) {
            if (c == '/') continue;

            if (isdigit(c)) {
                for (int i = 0; i < c - '0'; i++) {
                    squares[index] = Piece::NONE;
                    index++;
                }

                continue;
            }

            int piece = islower(c) ? Piece::BLACK : Piece::WHITE;

            switch (tolower(c)) {
                case 'p':
                    piece += Piece::PAWN;
                    break;
                case 'n':
                    piece += Piece::KNIGHT;
                    break;
                case 'b':
                    piece += Piece::BISHOP;
                    break;
                case 'r':
                    piece += Piece::ROOK;
                    break;
                case 'q':
                    piece += Piece::QUEEN;
                    break;
                case 'k':
                    piece += Piece::KING;
                    break;
                default:
                    throw std::invalid_argument("Invalid FEN! Not a valid character");
            }

            squares[index] = piece;
            index++;
        }

        colorToMove = (sideToMove == "w") ? Piece::WHITE : Piece::BLACK;

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
        return (7 - rank) * 8 + file;
    }

    bool Position::turn() const {
        return (colorToMove == Piece::WHITE);
    }

    void Position::flipTurn() {
        colorToMove = (colorToMove == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
    }
}
