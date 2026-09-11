#include "GenerateMoves.h"

#include <cstdlib>
#include <cstring>

#include "PMD.h"
#include "../Position/Piece.h"

namespace moveGeneration {
   static_assert(sizeof(position::Move) == 4, "Move must be packed into 4 bytes");

   position::Move MoveGenerator::moveBuffer[256];
   int MoveGenerator::moveCount{};
   int MoveGenerator::friendlyColor{};
   int MoveGenerator::oppositeColor{};

   std::vector<position::Move> MoveGenerator::generatePseudoLegalMoves(const position::Position& pos) {
      position::Move moves[256];
      int n = generatePseudoLegalMoves(pos, moves);
      return std::vector<position::Move>(moves, moves + n);
   }

   int MoveGenerator::generatePseudoLegalMoves(const position::Position& pos, position::Move* outMoves) {
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

      std::memcpy(outMoves, moveBuffer, moveCount * sizeof(position::Move));
      return moveCount;
   }

   std::vector<position::Move> MoveGenerator::generateLegalMoves(const position::Position& pos) {
      position::Move moves[256];
      position::Position work(pos);
      int n = generateLegalMoves(work, moves);
      return std::vector<position::Move>(moves, moves + n);
   }

   int MoveGenerator::generateLegalMoves(position::Position& pos, position::Move* outMoves) {
      position::Move pseudo[256];
      int pseudoCount = generatePseudoLegalMoves(pos, pseudo);

      if (pos.whiteKingSquare < 0 || pos.blackKingSquare < 0) return 0;

      int legalCount = 0;
      for (int i = 0; i < pseudoCount; i++) {
         const position::Move& mv = pseudo[i];
         if (mv.castle()) {
            int ks = friendlyColor == position::Piece::WHITE ? pos.whiteKingSquare : pos.blackKingSquare;
            if (isSquareAttacked(pos, ks, oppositeColor)) continue;

            bool kingSide = (mv.toSquare() % 8) == 6;
            int throughSquare = kingSide ? ks + 1 : ks - 1;
            int destSquare = kingSide ? ks + 2 : ks - 2;
            if (isSquareAttacked(pos, throughSquare, oppositeColor)) continue;
            if (isSquareAttacked(pos, destSquare, oppositeColor)) continue;
         }

         if (mv.toSquare() == pos.whiteKingSquare || mv.toSquare() == pos.blackKingSquare) continue;

         UndoInfo undo;
         makeMove(pos, mv, undo);
         int ks = friendlyColor == position::Piece::WHITE ? pos.whiteKingSquare : pos.blackKingSquare;
         bool legal = !isSquareAttacked(pos, ks, oppositeColor);
         unmakeMove(pos, mv, undo);

         if (legal) outMoves[legalCount++] = mv;
      }

      return legalCount;
   }

   std::uint64_t MoveGenerator::perft(position::Position& pos, int depth) {
      if (depth <= 1) {
         position::Move moves[256];
         return generateLegalMoves(pos, moves);
      }

      position::Move moves[256];
      int count = generateLegalMoves(pos, moves);

      std::uint64_t nodes = 0;
      for (int i = 0; i < count; i++) {
         UndoInfo undo;
         makeMove(pos, moves[i], undo);
         nodes += perft(pos, depth - 1);
         unmakeMove(pos, moves[i], undo);
      }

      return nodes;
   }

   void MoveGenerator::makeMove(position::Position& pos, const position::Move& move, UndoInfo& undo) {
      undo.movedPiece = pos.squares[move.fromSquare()];
      undo.capturedPiece = pos.squares[move.toSquare()];
      undo.enPassantSquare = pos.enPassantSquare;
      undo.castlingRights = pos.castlingRights;

      int color = position::Piece::getPieceColor(undo.movedPiece);
      int type  = position::Piece::getPieceType(undo.movedPiece);

      pos.squares[move.toSquare()] = undo.movedPiece;
      pos.squares[move.fromSquare()] = position::Piece::NONE;

      if (move.enPassant()) {
         int capturedPawnSquare = move.toSquare() + ((color == position::Piece::WHITE) ? 8 : -8);
         undo.capturedPiece = pos.squares[capturedPawnSquare];
         pos.squares[capturedPawnSquare] = position::Piece::NONE;
      }

      if (move.promotion() != position::Piece::NONE) {
         pos.squares[move.toSquare()] = color + move.promotion();
      }

      if (move.castle()) {
         if (move.toSquare() % 8 == 6) {
            pos.squares[move.toSquare() - 1] = color + position::Piece::ROOK;
            pos.squares[move.toSquare() + 1] = position::Piece::NONE;
         } else {
            pos.squares[move.toSquare() + 1] = color + position::Piece::ROOK;
            pos.squares[move.toSquare() - 2] = position::Piece::NONE;
         }
      }

      if (type == position::Piece::PAWN && std::abs(move.toSquare() - move.fromSquare()) == 16) {
         pos.enPassantSquare = move.fromSquare() + ((color == position::Piece::WHITE) ? -8 : 8);
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

      if (move.fromSquare() == 56 || move.toSquare() == 56) pos.castlingRights &= ~position::Position::WHITE_QUEENSIDE;
      if (move.fromSquare() == 63 || move.toSquare() == 63) pos.castlingRights &= ~position::Position::WHITE_KINGSIDE;
      if (move.fromSquare() == 0  || move.toSquare() == 0)  pos.castlingRights &= ~position::Position::BLACK_QUEENSIDE;
      if (move.fromSquare() == 7  || move.toSquare() == 7)  pos.castlingRights &= ~position::Position::BLACK_KINGSIDE;

      if (type == position::Piece::KING) {
         if (color == position::Piece::WHITE) pos.whiteKingSquare = move.toSquare();
         else pos.blackKingSquare = move.toSquare();
      }

      pos.flipTurn();
   }

   void MoveGenerator::unmakeMove(position::Position& pos, const position::Move& move, const UndoInfo& undo) {
      int color = position::Piece::getPieceColor(undo.movedPiece);
      int type  = position::Piece::getPieceType(undo.movedPiece);

      pos.squares[move.fromSquare()] = undo.movedPiece;

      if (move.castle()) {
         if (move.toSquare() % 8 == 6) {
            pos.squares[move.toSquare() - 1] = position::Piece::NONE;
            pos.squares[move.toSquare() + 1] = color + position::Piece::ROOK;
         } else {
            pos.squares[move.toSquare() + 1] = position::Piece::NONE;
            pos.squares[move.toSquare() - 2] = color + position::Piece::ROOK;
         }
      }

      pos.squares[move.toSquare()] = undo.capturedPiece;

      if (move.enPassant()) {
         int capturedPawnSquare = move.toSquare() + ((color == position::Piece::WHITE) ? 8 : -8);
         pos.squares[capturedPawnSquare] = undo.capturedPiece;
         pos.squares[move.toSquare()] = position::Piece::NONE;
      }

      if (type == position::Piece::KING) {
         if (color == position::Piece::WHITE) pos.whiteKingSquare = move.fromSquare();
         else pos.blackKingSquare = move.fromSquare();
      }

      if (position::Piece::getPieceType(undo.capturedPiece) == position::Piece::KING) {
         if (position::Piece::getPieceColor(undo.capturedPiece) == position::Piece::WHITE) {
            pos.whiteKingSquare = move.toSquare();
         } else {
            pos.blackKingSquare = move.toSquare();
         }
      }

      pos.enPassantSquare = undo.enPassantSquare;
      pos.castlingRights = undo.castlingRights;
      pos.flipTurn();
   }

   void MoveGenerator::applyMove(position::Position& pos, const position::Move& move) {
      UndoInfo discard;
      makeMove(pos, move, discard);
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
            if (pieceOnTargetSquare != position::Piece::NONE) moveBuffer[moveCount - 1].setCapture();

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
                  for (int k = moveCount - 4; k < moveCount; k++) moveBuffer[k].setCapture();
               } else {
                  moveBuffer[moveCount++] = position::Move(i, target, false, isEp);
                  moveBuffer[moveCount - 1].setCapture();
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
            if (pos.squares[target] != position::Piece::NONE) moveBuffer[moveCount - 1].setCapture();
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
            if (pos.squares[target] != position::Piece::NONE) moveBuffer[moveCount - 1].setCapture();
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
}
