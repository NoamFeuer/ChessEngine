#include "GenerateMoves.h"

#include <cstdlib>

#include "PMD.h"
#include "../Position/Piece.h"

namespace moveGeneration {
   position::Move MoveGenerator::moveBuffer[256];
   int MoveGenerator::moveCount{};
   int MoveGenerator::friendlyColor{};
   int MoveGenerator::oppositeColor{};

   std::vector<position::Move> MoveGenerator::generatePseudoLegalMoves(const position::Position& pos) {
      std::vector<position::Move> moves;

      moveCount = 0;
      friendlyColor = (pos.turn()) ? position::Piece::WHITE : position::Piece::BLACK;
      oppositeColor = (pos.turn()) ? position::Piece::BLACK : position::Piece::WHITE;

      for (int i = 0; i < 64; i++) {
         int piece = pos.squares[i];
         if (piece == position::Piece::NONE) continue;
         if (!position::Piece::isColor(piece, friendlyColor)) continue;

         int type = position::Piece::getPieceType(piece);
         if (type == position::Piece::BISHOP ||
             type == position::Piece::ROOK ||
             type == position::Piece::QUEEN) {
            generateSlidingMoves(pos, i, piece);
         }
      }

      generatePawnMoves(pos);
      generateKnightMoves(pos);
      generateKingMoves(pos);
      generateCastlingMoves(pos);

      for (int i = 0; i < moveCount; i++) {
         moves.push_back(moveBuffer[i]);
      }

      return moves;
   }

   std::vector<position::Move> MoveGenerator::generateLegalMoves(const position::Position& pos) {
      std::vector<position::Move> pseudo = generatePseudoLegalMoves(pos);
      std::vector<position::Move> legal;

      for (const position::Move& move : pseudo) {
         if (move.castle) {
            int kingSquare = findKingSquare(pos, friendlyColor);
            if (isSquareAttacked(pos, kingSquare, oppositeColor)) continue;

            bool kingSide = (move.toSquare % 8) == 6;
            int throughSquare = kingSide ? kingSquare + 1 : kingSquare - 1;
            int destSquare = kingSide ? kingSquare + 2 : kingSquare - 2;
            if (isSquareAttacked(pos, throughSquare, oppositeColor)) continue;
            if (isSquareAttacked(pos, destSquare, oppositeColor)) continue;
         }

         position::Position copy(pos);
         applyMove(copy, move);
         if (findKingSquare(copy, oppositeColor) < 0) continue;
         int ourKing = findKingSquare(copy, friendlyColor);
         if (!isSquareAttacked(copy, ourKing, oppositeColor)) {
            legal.push_back(move);
         }
      }

      return legal;
   }

   std::uint64_t MoveGenerator::perft(const position::Position& pos, int depth) {
      std::vector<position::Move> moves = generateLegalMoves(pos);
      if (depth <= 1) return moves.size();

      std::uint64_t nodes = 0;
      for (const position::Move& move : moves) {
         position::Position copy(pos);
         applyMove(copy, move);
         nodes += perft(copy, depth - 1);
      }

      return nodes;
   }

   void MoveGenerator::generateSlidingMoves(const position::Position& pos, int startSquare, int piece) {
      int startDirIndex = position::Piece::isType(piece, position::Piece::BISHOP) ? 4 : 0;
      int endDirIndex = position::Piece::isType(piece, position::Piece::ROOK)   ? 4 : 8;

      for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
         for (int n = 0; n < PMD::numSquaresToEdge[startSquare][directionIndex]; n++) {
            int targetSquare = startSquare + PMD::directionOffsets[directionIndex] * (n + 1);
            int pieceOnTargetSquare = pos.squares[targetSquare];

            if (position::Piece::isColor(pieceOnTargetSquare, friendlyColor)) break;

            moveBuffer[moveCount++] = position::Move(startSquare, targetSquare);
            if (pieceOnTargetSquare != position::Piece::NONE) moveBuffer[moveCount - 1].capture = true;

            if (position::Piece::isColor(pieceOnTargetSquare, oppositeColor)) break;
         }
      }
   }

   void MoveGenerator::generatePawnMoves(const position::Position& pos) {
      bool white = (friendlyColor == position::Piece::WHITE);
      int forward = white ? -8 : 8;
      int startRank = white ? 6 : 1;
      int promoRank = white ? 1 : 6;

      for (int i = 0; i < 64; i++) {
         int piece = pos.squares[i];
         if (!position::Piece::isType(piece, position::Piece::PAWN)) continue;
         if (!position::Piece::isColor(piece, friendlyColor)) continue;

         int rank = i / 8;
         int file = i % 8;

         int oneStep = i + forward;
         if (oneStep >= 0 && oneStep < 64 && pos.squares[oneStep] == position::Piece::NONE) {
            if (rank == promoRank) {
               moveBuffer[moveCount++] = position::Move(i, oneStep, false, false, position::Piece::QUEEN);
               moveBuffer[moveCount++] = position::Move(i, oneStep, false, false, position::Piece::ROOK);
               moveBuffer[moveCount++] = position::Move(i, oneStep, false, false, position::Piece::BISHOP);
               moveBuffer[moveCount++] = position::Move(i, oneStep, false, false, position::Piece::KNIGHT);
            } else {
               moveBuffer[moveCount++] = position::Move(i, oneStep);
            }

            int twoStep = i + 2 * forward;
            if (rank == startRank && pos.squares[twoStep] == position::Piece::NONE) {
               moveBuffer[moveCount++] = position::Move(i, twoStep);
            }
         }

         int captureOffsets[2] = { forward - 1, forward + 1 };
         int captureFiles[2] = { file - 1, file + 1 };
         for (int c = 0; c < 2; c++) {
            if (captureFiles[c] < 0 || captureFiles[c] > 7) continue;

            int target = i + captureOffsets[c];
            int targetPiece = pos.squares[target];
            bool isCapture = position::Piece::isColor(targetPiece, oppositeColor);
            bool isEp = (target == pos.enPassantSquare);

            if (isCapture || isEp) {
               if (rank == promoRank) {
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp, position::Piece::QUEEN);
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp, position::Piece::ROOK);
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp, position::Piece::BISHOP);
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp, position::Piece::KNIGHT);
                  for (int k = moveCount - 4; k < moveCount; k++) moveBuffer[k].capture = true;
               } else {
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp);
                  moveBuffer[moveCount - 1].capture = true;
               }
            }
         }
      }
   }

   void MoveGenerator::generateKnightMoves(const position::Position& pos) {
      for (int p = 0; p < 64; p++) {
         int piece = pos.squares[p];
         if (!position::Piece::isType(piece, position::Piece::KNIGHT)) continue;
         if (!position::Piece::isColor(piece, friendlyColor)) continue;

         int rank = p / 8;
         int file = p % 8;

         for (int m = 0; m < 8; m++) {
            int targetRank = rank + PMD::knightRankOffsets[m];
            int targetFile = file + PMD::knightFileOffsets[m];
            if (targetRank < 0 || targetRank > 7 || targetFile < 0 || targetFile > 7) continue;

            int target = p + PMD::knightOffsets[m];
            if (position::Piece::isColor(pos.squares[target], friendlyColor)) continue;

            moveBuffer[moveCount++] = position::Move(p, target);
            if (pos.squares[target] != position::Piece::NONE) moveBuffer[moveCount - 1].capture = true;
         }
      }
   }

   void MoveGenerator::generateKingMoves(const position::Position& pos) {
      for (int p = 0; p < 64; p++) {
         int piece = pos.squares[p];
         if (!position::Piece::isType(piece, position::Piece::KING)) continue;
         if (!position::Piece::isColor(piece, friendlyColor)) continue;

         int rank = p / 8;
         int file = p % 8;

         for (int d = 0; d < 8; d++) {
            int target = p + PMD::directionOffsets[d];
            if (target < 0 || target > 63) continue;

            int tRank = target / 8;
            int tFile = target % 8;
            if (std::abs(tRank - rank) > 1 || std::abs(tFile - file) > 1) continue;

            if (position::Piece::isColor(pos.squares[target], friendlyColor)) continue;

            moveBuffer[moveCount++] = position::Move(p, target);
            if (pos.squares[target] != position::Piece::NONE) moveBuffer[moveCount - 1].capture = true;
         }
      }
   }

   void MoveGenerator::generateCastlingMoves(const position::Position& pos) {
      if (friendlyColor == position::Piece::WHITE) {
         int kingSquare = 60;
         if (pos.squares[kingSquare] != position::Piece::WHITE + position::Piece::KING) return;

         if ((pos.castlingRights & position::Position::WHITE_KINGSIDE) &&
             pos.squares[61] == position::Piece::NONE &&
             pos.squares[62] == position::Piece::NONE) {
            moveBuffer[moveCount++] = position::Move(kingSquare, 62, true);
         }

         if ((pos.castlingRights & position::Position::WHITE_QUEENSIDE) &&
             pos.squares[59] == position::Piece::NONE &&
             pos.squares[58] == position::Piece::NONE &&
             pos.squares[57] == position::Piece::NONE) {
            moveBuffer[moveCount++] = position::Move(kingSquare, 58, true);
         }
      } else {
         int kingSquare = 4;
         if (pos.squares[kingSquare] != position::Piece::BLACK + position::Piece::KING) return;

         if ((pos.castlingRights & position::Position::BLACK_KINGSIDE) &&
             pos.squares[5] == position::Piece::NONE &&
             pos.squares[6] == position::Piece::NONE) {
            moveBuffer[moveCount++] = position::Move(kingSquare, 6, true);
         }

         if ((pos.castlingRights & position::Position::BLACK_QUEENSIDE) &&
             pos.squares[3] == position::Piece::NONE &&
             pos.squares[2] == position::Piece::NONE &&
             pos.squares[1] == position::Piece::NONE) {
            moveBuffer[moveCount++] = position::Move(kingSquare, 2, true);
         }
      }
   }

   int MoveGenerator::findKingSquare(const position::Position& pos, int color) {
      for (int i = 0; i < 64; i++) {
         int piece = pos.squares[i];
         if (position::Piece::isType(piece, position::Piece::KING) &&
             position::Piece::isColor(piece, color)) {
            return i;
         }
      }

      return -1;
   }

   bool MoveGenerator::isSquareAttacked(const position::Position& pos, int square, int attackerColor) {
      if (square < 0) return true;
      int rank = square / 8;
      int file = square % 8;

      if (attackerColor == position::Piece::WHITE) {
         if (rank <= 6) {
            if (file > 0 && pos.squares[square + 7] == position::Piece::WHITE + position::Piece::PAWN) return true;
            if (file < 7 && pos.squares[square + 9] == position::Piece::WHITE + position::Piece::PAWN) return true;
         }
      } else {
         if (rank >= 1) {
            if (file > 0 && pos.squares[square - 9] == position::Piece::BLACK + position::Piece::PAWN) return true;
            if (file < 7 && pos.squares[square - 7] == position::Piece::BLACK + position::Piece::PAWN) return true;
         }
      }

      for (int m = 0; m < 8; m++) {
         int targetRank = rank + PMD::knightRankOffsets[m];
         int targetFile = file + PMD::knightFileOffsets[m];
         if (targetRank < 0 || targetRank > 7 || targetFile < 0 || targetFile > 7) continue;

         int target = square + PMD::knightOffsets[m];
         if (pos.squares[target] == attackerColor + position::Piece::KNIGHT) return true;
      }

      for (int d = 0; d < 8; d++) {
         int target = square + PMD::directionOffsets[d];
         if (target < 0 || target > 63) continue;

         int tRank = target / 8;
         int tFile = target % 8;
         if (std::abs(tRank - rank) > 1 || std::abs(tFile - file) > 1) continue;

         if (pos.squares[target] == attackerColor + position::Piece::KING) return true;
      }

      for (int d = 0; d < 8; d++) {
         for (int n = 0; n < PMD::numSquaresToEdge[square][d]; n++) {
            int target = square + PMD::directionOffsets[d] * (n + 1);
            int piece = pos.squares[target];
            if (piece == position::Piece::NONE) continue;

            if (!position::Piece::isColor(piece, attackerColor)) break;

            int type = position::Piece::getPieceType(piece);
            if (type == position::Piece::QUEEN) return true;
            if (d < 4 && type == position::Piece::ROOK) return true;
            if (d >= 4 && type == position::Piece::BISHOP) return true;
            break;
         }
      }

      return false;
   }

   void MoveGenerator::applyMove(position::Position& pos, const position::Move& move) {
      int piece = pos.squares[move.fromSquare];
      int type = position::Piece::getPieceType(piece);
      int color = position::Piece::getPieceColor(piece);

      pos.squares[move.toSquare] = piece;
      pos.squares[move.fromSquare] = position::Piece::NONE;

      if (move.enPassant) {
         int capturedPawnSquare = move.toSquare + ((color == position::Piece::WHITE) ? 8 : -8);
         pos.squares[capturedPawnSquare] = position::Piece::NONE;
      }

      if (move.promotion != position::Piece::NONE) {
         pos.squares[move.toSquare] = color + move.promotion;
      }

      if (move.castle) {
         if (move.toSquare % 8 == 6) {
            pos.squares[move.toSquare - 1] = color + position::Piece::ROOK;
            pos.squares[move.toSquare + 1] = position::Piece::NONE;
         } else {
            pos.squares[move.toSquare + 1] = color + position::Piece::ROOK;
            pos.squares[move.toSquare - 2] = position::Piece::NONE;
         }
      }

      if (type == position::Piece::PAWN && std::abs(move.toSquare - move.fromSquare) == 16) {
         pos.enPassantSquare = move.fromSquare + ((color == position::Piece::WHITE) ? -8 : 8);
      } else {
         pos.enPassantSquare = -1;
      }

      if (type == position::Piece::KING) {
         if (color == position::Piece::WHITE) {
            pos.castlingRights &= ~(position::Position::WHITE_KINGSIDE | position::Position::WHITE_QUEENSIDE);
         } else {
            pos.castlingRights &= ~(position::Position::BLACK_KINGSIDE | position::Position::BLACK_QUEENSIDE);
         }
      }

      if (move.fromSquare == 56 || move.toSquare == 56) pos.castlingRights &= ~position::Position::WHITE_QUEENSIDE;
      if (move.fromSquare == 63 || move.toSquare == 63) pos.castlingRights &= ~position::Position::WHITE_KINGSIDE;
      if (move.fromSquare == 0  || move.toSquare == 0)  pos.castlingRights &= ~position::Position::BLACK_QUEENSIDE;
      if (move.fromSquare == 7  || move.toSquare == 7)  pos.castlingRights &= ~position::Position::BLACK_KINGSIDE;

      pos.flipTurn();
   }
}