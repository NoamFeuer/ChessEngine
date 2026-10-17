#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <chrono>

#include "Position/Position.h"
#include "Position/Piece.h"
#include "MoveGeneration/GenerateMoves.h"
#include "Search/Search.h"

using namespace position;
using namespace moveGeneration;

constexpr int SEARCH_DEPTH = 6;

// Convert a square index (0-63) to chess notation.
std::string squareToString(int square) {
    std::string result = "a1";
    result[0] = 'a' + (square % 8);
    result[1] = '1' + (square / 8);

    return result;
}

// Convert chess notation to a square index.
int stringToSquare(const std::string& s, int offset) {
    if (offset + 1 >= static_cast<int>(s.size())) return -1;

    char file = static_cast<char>(std::tolower(static_cast<unsigned char>(s[offset])));
    char rank = s[offset + 1];

    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;

    return (rank - '1') * 8 + (file - 'a');
}

char pieceSymbol(const Position& pos, int square) {
    Bitboard bit = bitBoardOf(square);

    int type = 0;
    for (int t = Piece::PAWN; t <= Piece::KING; ++t) {
        if (pos.byType[t] & bit) {
            type = t;
            break;
        }
    }

    if (type == 0) return '.';

    char symbol = '?';

    switch (type) {
        case Piece::PAWN:   symbol = 'p'; break;
        case Piece::KNIGHT: symbol = 'n'; break;
        case Piece::BISHOP: symbol = 'b'; break;
        case Piece::ROOK:   symbol = 'r'; break;
        case Piece::QUEEN:  symbol = 'q'; break;
        case Piece::KING:   symbol = 'k'; break;
    }

    if (pos.byColor[WHITE_INDEX] & bit)
        symbol = static_cast<char>(std::toupper(static_cast<unsigned char>(symbol)));

    return symbol;
}

void printBoard(const Position& pos) {
    std::cout << "\n    a b c d e f g h\n";
    std::cout << "  +-----------------+\n";

    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";

        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            std::cout << pieceSymbol(pos, square) << ' ';
        }

        std::cout << "| " << rank + 1 << '\n';
    }

    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
    std::cout << "White = uppercase, Black = lowercase\n";
}

// Parse promotion suffixes such as q, r, b, n.
int parsePromotion(const std::string& input) {
    if (input.size() < 5)
        return Piece::NONE;

    switch (std::tolower(
        static_cast<unsigned char>(input[4]))) {
        case 'q': return Piece::QUEEN;
        case 'r': return Piece::ROOK;
        case 'b': return Piece::BISHOP;
        case 'n': return Piece::KNIGHT;
        default:  return -1;
    }
}

// Accepts moves such as e2e4, e7e8q, or g1f3.
bool parseMove(const Position& pos, const std::string& input,Move& chosenMove) {
    if (input.size() != 4 && input.size() != 5) return false;

    int from = stringToSquare(input, 0);
    int to = stringToSquare(input, 2);

    if (from < 0 || to < 0) return false;

    int promotion = parsePromotion(input);
    if (promotion == -1) return false;

    std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(pos);

    for (const Move& move : legalMoves) {
        if (move.fromSquare() != from ||
            move.toSquare() != to) {
            continue;
        }

        if (move.promotion() != promotion) continue;

        chosenMove = move;
        return true;
    }

    return false;
}

bool isInCheck(const Position& pos) {
    bool whiteToMove = pos.turn();

    int kingIndex = whiteToMove ? WHITE_INDEX : BLACK_INDEX;
    int enemyColor = whiteToMove ? Piece::BLACK : Piece::WHITE;
    int kingSquare = pos.kingSquare(kingIndex);

    return MoveGenerator::isSquareAttacked(pos, kingSquare, enemyColor);
}

int main() {
    Position pos;

    std::cout << "============================\n";
    std::cout << "     CONSOLE CHESS BOT\n";
    std::cout << "============================\n";
    std::cout << "You play White. The bot plays Black.\n";
    std::cout << "Enter moves like e2e4 or e7e8q.\n";
    std::cout << "Type 'quit' to exit.\n";

    while (true) {
        printBoard(pos);

        // Generate moves to detect checkmate or stalemate.
        std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(pos);

        if (legalMoves.empty()) {
            if (isInCheck(pos))
                std::cout << "Checkmate! " << (pos.turn() ? "Black" : "White") << " wins.\n";
            else
                std::cout << "Draw by stalemate!\n";

            break;
        }

        if (isInCheck(pos))
            std::cout << "Check!\n";

        if (pos.turn()) {
            std::string input;
            Move move;

            std::cout << "Your move: ";
            std::cin >> input;

            if (!std::cin)
                break;

            if (input == "quit")
                break;

            if (!parseMove(pos, input, move)) {
                std::cout << "Invalid move. Try again.\n";
                continue;
            }

            MoveGenerator::applyMove(pos, move);
        }
        else {
            std::cout << "Bot is thinking...\n";

            const auto start = std::chrono::steady_clock::now();
            search::SearchResult result = search::findBestMove(pos, SEARCH_DEPTH);

            const auto end = std::chrono::steady_clock::now();
            const auto thinkingTime = std::chrono::duration<double>(end - start).count();

            std::cout << "Bot plays " << squareToString(result.bestMove.fromSquare()) << squareToString(result.bestMove.toSquare());

            if (result.bestMove.promotion() != Piece::NONE) {
                char promo = '?';

                switch (result.bestMove.promotion()) {
                    case Piece::QUEEN:  promo = 'q'; break;
                    case Piece::ROOK:   promo = 'r'; break;
                    case Piece::BISHOP: promo = 'b'; break;
                    case Piece::KNIGHT: promo = 'n'; break;
                }

                std::cout << promo;
            }

            std::cout << "\nEvaluation: " << result.score << "\nNodes searched: " << result.nodes << "\nThinking time: " << thinkingTime << " seconds\n";

            MoveGenerator::applyMove(pos, result.bestMove);
        }
    }

    std::cout << "Game over. Thanks for playing!\n";
    return 0;
}
