#include <iostream>
#include "Position/Position.h"
#include "Position/Piece.h"
#include "MoveGeneration/GenerateMoves.h"

int main() {
    std::cout << "WELCOME TO MY CHESS ENGINE!" << std::endl;

    position::Position pos("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");

    auto pseudoWhite = moveGeneration::MoveGenerator::generatePseudoLegalMoves(pos);
    auto legalWhite = moveGeneration::MoveGenerator::generateLegalMoves(pos);

    std::cout << "White pseudo-legal moves: " << pseudoWhite.size() << std::endl; 
    std::cout << "White legal moves: " << legalWhite.size() << std::endl;

    position::Position blackPos(pos);
    blackPos.flipTurn();

    auto pseudoBlack = moveGeneration::MoveGenerator::generatePseudoLegalMoves(blackPos);
    auto legalBlack = moveGeneration::MoveGenerator::generateLegalMoves(blackPos);

    std::cout << "Black pseudo-legal moves: " << pseudoBlack.size() << std::endl;
    std::cout << "Black legal moves: " << legalBlack.size() << std::endl;

    std::cout << "\nPerft (validated) from the initial position:" << std::endl;
    for (int depth = 1; depth <= 4; depth++) {
        std::cout << "  depth " << depth << ": " << moveGeneration::MoveGenerator::perft(pos, depth) << std::endl;
    }

    return 0;
}
