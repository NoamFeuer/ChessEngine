#include <iostream>
#include "Position/Position.h"
#include "Position/Piece.h"
#include "MoveGeneration/GenerateMoves.h"

int main() {
    std::cout << "WELCOME TO MY CHESS ENGINE!" << std::endl;

    position::Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");

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
    for (int depth = 1; depth <= 6; depth++) {
        std::cout << "  depth " << depth << ": " << moveGeneration::MoveGenerator::perft(pos, depth) << std::endl;
    }

    return 0;
}
