#include "../include/Globals.hpp"
#include "../include/Board.hpp"
#include "../include/BitMath.hpp"
#include "../include/BitBoards.hpp"
#include "Board.hpp"

void encodePawnMove(MoveList& move_list, BitBoard bb, int shift_amount, MoveCode code) {
    while (bb) {
        Square to = popLSB(bb);
        move_list.add((to-shift_amount) | (to << 6) | (code << 12));
    }
}

void encodePromotion(MoveList& move_list, BitBoard bb, int shift_amount, bool is_capture) {
    MoveCode base_code = is_capture ? c_npromo : npromo;
    while (bb) {
        Square to = popLSB(bb);
        int from = to - shift_amount;
        move_list.add(from | (to << 6) | (base_code << 12));
        move_list.add(from | (to << 6) | ((base_code + 1) << 12));
        move_list.add(from | (to << 6) | ((base_code + 2) << 12));
        move_list.add(from | (to << 6) | ((base_code + 3) << 12));
    }
}

template <Colour us, GenType type>
void generatePawnMoves(const Board& board, MoveList& move_list, BitBoard target) {
    constexpr Colour them = (us == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;
    constexpr BitBoard _rank7 = (us == Colour::WHITE) ? rank7BB : rank2BB;
    constexpr BitBoard _rank3 = (us == Colour::WHITE) ? rank3BB : rank6BB;
    constexpr Direction up = (us == Colour::WHITE) ? Direction::North : Direction::South;
    constexpr Direction up_left = (us == Colour::WHITE) ? Direction::NW : Direction::SE;
    constexpr Direction up_right = (us == Colour::WHITE) ? Direction::NE : Direction::SW;

    const BitBoard empty = ~board.bitBoards[allpieces];
    const BitBoard enemy = (type == GenType::EVASIONS) ? board.checkers() 
                        : board.bitBoards[them == Colour::WHITE ? wpieces : bpieces];

    BitBoard pawns7 = board.bitBoards[us == Colour::WHITE ? P : p] & _rank7;
    BitBoard pawnsnot7 = board.bitBoards[us == Colour::WHITE ? P : p] & ~_rank7;

    // Single and dbl (no promo)
    if constexpr (type != GenType::CAPTURES) {
        BitBoard single = shift<up>(pawnsnot7) & empty;
        BitBoard double_ = shift<up>(single & _rank3) & empty;

        if constexpr (type == GenType::EVASIONS) {
            // because target = blockers | checkers
            single &= target;
            double_ &= target;
        }

        encodePawnMove(move_list, single, shift_val[static_cast<int>(up)], quiet);
        encodePawnMove(move_list, double_, shift_val[static_cast<int>(up)] * 2, dblpush);
    }

    // Promotions
    if (pawns7) {
        BitBoard b1 = shift<up_right>(pawns7) & enemy;
        BitBoard b2 = shift<up_left>(pawns7) & enemy;
        BitBoard b3 = shift<up>(pawns7) & empty;

        if constexpr (type == GenType::EVASIONS) {
            b3 &= target;
            // b1 and b2 are already filetered above: enemy = target
        }

        if constexpr (type != GenType::QUIET) {
            encodePromotion(move_list, b1, shift_val[static_cast<int>(up_right)], true);
            encodePromotion(move_list, b2, shift_val[static_cast<int>(up_left)], true);
        }

        encodePromotion(move_list, b3, shift_val[static_cast<int>(up)], false);
    }

    // Standard + ep
    if constexpr(type != GenType::QUIET) {
        BitBoard b1 = shift<up_right>(pawnsnot7) & enemy;
        BitBoard b2 = shift<up_left>(pawnsnot7) & enemy;

        encodePawnMove(move_list, b1, shift_val[static_cast<int>(up_right)], capture);
        encodePawnMove(move_list, b2, shift_val[static_cast<int>(up_left)], capture);

        if (board.currentState.epSquare != no_square) {
            if (type == GenType::EVASIONS && (target & (board.currentState.epSquare + static_cast<int>(up)))) {
                return; // Ep can't resolve discovered check == 0
            }

            b1 = pawnsnot7 & attacks<(us == Colour::WHITE ? p : P)>(board.currentState.epSquare);

            assert(b1);
            while (b1) {
                Square from = popLSB(b1);
                move_list.add(from | (board.currentState.epSquare << 6) | (epcapture << 12));
            }
        }
    }
}

template <Piece piece>
void encodeMoves(MoveList& move_list, Square from, BitBoard bb, BitBoard otherpieces) {
    static_assert(piece != P && piece != p && piece != K && piece != k); // not supported
    MoveCode code;
    while (bb) {
        Square to = popLSB(bb);
        if (otherpieces & mask(to)) code = capture;
        else code = quiet;
        move_list.add((from) | (to << 6) | (code << 12));
    }
}

template <Piece piece>
void generateOthers(const Board& board, MoveList& move_list, BitBoard target) {
    static_assert(piece != P && piece != p && piece != K && piece != k); // not supported
    BitBoard bb = board.bitBoards[piece];
    BitBoard others = board.bitBoards[piece <= k ? wpieces : bpieces];
    while (bb) {
        Square from = popLSB(bb);
        BitBoard b = attacks<piece>(from) & target;
        encodeMoves<piece>(move_list, from, b, others);
    }
}

template <Colour us, GenType type>
void generateAllPieces(const Board& board, MoveList& move_list) {
    BitBoard target;
    if (type != GenType::EVASIONS || !board.moreThanOneChecker()) {
        target = type == GenType::CAPTURES ? board.bitBoards[us == Colour::WHITE ? bpieces : wpieces]
                : type == GenType::ALL ? ~board.bitBoards[us == Colour::WHITE ? wpieces : bpieces] 
                : type == GenType::QUIET ? ~board.bitBoards[allpieces] 
                : type == GenType::EVASIONS ? board.checkers() // | blockers TODO
                : 0;

        generatePawnMoves<us, type>(board, move_list, target);
        if (us == Colour::WHITE) {
            generateOthers<N>(board, move_list, target);
            generateOthers<B>(board, move_list, target);
            generateOthers<R>(board, move_list, target);
            generateOthers<Q>(board, move_list, target);
        } else {
            generateOthers<n>(board, move_list, target);
            generateOthers<b>(board, move_list, target);
            generateOthers<r>(board, move_list, target);
            generateOthers<q>(board, move_list, target);
        }
    }

    // gen king and castling
}

template <GenType type>
void Board::generateMoves(MoveList& move_list) const {
    assert(type != GenType::LEGAL); // not supported

    Colour us = sideToMove;
    if (us == Colour::WHITE) {
        generateAllPieces<Colour::WHITE, type>(*this, move_list);
    } else {
        generateAllPieces<Colour::BLACK, type>(*this, move_list);
    }
}

template void Board::generateMoves<GenType::ALL>(MoveList& move_list) const;
template void Board::generateMoves<GenType::CAPTURES>(MoveList& move_list) const;
template void Board::generateMoves<GenType::QUIET>(MoveList& move_list) const;
template void Board::generateMoves<GenType::EVASIONS>(MoveList& move_list) const;

template <>
void Board::generateMoves<GenType::LEGAL>(MoveList& move_list) const {
    // Generate legal moves
    // TODO implement this
}