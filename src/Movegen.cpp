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
    constexpr Colour them = oppC(us);
    constexpr BitBoard _rank7 = (us == Colour::WHITE) ? rank7BB : rank2BB;
    constexpr BitBoard _rank3 = (us == Colour::WHITE) ? rank3BB : rank6BB;
    constexpr Direction up = (us == Colour::WHITE) ? Direction::North : Direction::South;
    constexpr Direction up_left = (us == Colour::WHITE) ? Direction::NW : Direction::SE;
    constexpr Direction up_right = (us == Colour::WHITE) ? Direction::NE : Direction::SW;

    const BitBoard empty = ~board.piecesGet(allpieces);
    const BitBoard enemy = (type == GenType::EVASIONS) ? board.checkers() 
                        : board.piecesOf<them>(no_piece);

    BitBoard pawns7 = board.piecesOf<us>(Pc_p) & _rank7;
    BitBoard pawnsnot7 = board.piecesOf<us>(Pc_p) & ~_rank7;

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

            b1 = pawnsnot7 & attacks<(us == Colour::WHITE ? Pc_p : Pc_P)>(board.currentState.epSquare);

            assert(b1);
            while (b1) {
                Square from = popLSB(b1);
                move_list.add(from | (board.currentState.epSquare << 6) | (epcapture << 12));
            }
        }
    }
}

void encodeMoves(MoveList& move_list, Square from, BitBoard bb, BitBoard otherpieces) {
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
    static_assert(piece != Pc_P && piece != Pc_p && piece != Pc_K && piece != Pc_k); // not supported
    BitBoard bb = board.piecesGet(piece);
    const Colour them = piece <= Pc_k ? Colour::BLACK : Colour::WHITE;
    BitBoard others = board.piecesOf<them>(no_piece);
    [[maybe_unused]] BitBoard blockers = 0ULL;
    constexpr bool is_slider = 
    (piece == Pc_B || piece == Pc_b || piece == Pc_R || piece == Pc_r || piece == Pc_Q || piece == Pc_q);
    if constexpr (is_slider) {
        blockers = board.piecesGet(allpieces);
    }

    while (bb) {
        Square from = popLSB(bb);
        BitBoard attacks = 0ULL;
        if constexpr (is_slider)
            attacks = attacks<piece>(from, blockers) & target;
        else 
            attacks = attacks<piece>(from) & target;
        encodeMoves(move_list, from, attacks, others);
    }
}

void encodeCastling(MoveList& move_list, Colour c, bool is_king_side) {
    Square from, to;
    MoveCode code;
    if (is_king_side) code = MoveCode::kcastle;
    else code = MoveCode::qcastle;
    if (c == Colour::WHITE) {
        from = SQ_e1;
        if (is_king_side) {
            to = SQ_g1;
        } else {
            to = SQ_c1;
        }
    } else {
        from = SQ_e8;
        if (is_king_side) {
            to = SQ_g8;
        } else {
            to = SQ_c8;
        }
    }

    move_list.add((from) | (to << 6) | (code << 12));
}

template <Colour us, GenType type>
void generateAllPieces(const Board& board, MoveList& move_list) {
    BitBoard target;
    const Colour them = oppC(us);
    Square k_sq = lsb(board.piecesOf<us>(Pc_k));
    if (type != GenType::EVASIONS || !moreThanOne(board.checkers())) {
        target = type == GenType::CAPTURES ? board.piecesOf<them>(no_piece)
                : type == GenType::NON_EVASIONS ? ~board.piecesOf<us>(no_piece)
                : type == GenType::QUIET ? ~board.piecesGet(allpieces)
                : type == GenType::EVASIONS ? betweenBB[k_sq][lsb(board.checkers())]
                : 0;

        generatePawnMoves<us, type>(board, move_list, target);
        if (us == Colour::WHITE) {
            generateOthers<Pc_N>(board, move_list, target);
            generateOthers<Pc_B>(board, move_list, target);
            generateOthers<Pc_R>(board, move_list, target);
            generateOthers<Pc_Q>(board, move_list, target);
        } else {
            generateOthers<Pc_n>(board, move_list, target);
            generateOthers<Pc_b>(board, move_list, target);
            generateOthers<Pc_r>(board, move_list, target);
            generateOthers<Pc_q>(board, move_list, target);
        }
    }


    BitBoard bb = attacks<Pc_k>(k_sq) & (type == GenType::EVASIONS ? ~board.piecesOf<us>(no_piece) : target);
    encodeMoves(move_list, k_sq, bb, board.piecesOf<them>(no_piece));

    if ((type == GenType::QUIET || type == GenType::NON_EVASIONS) 
    && (board.canCastle(us, true) || board.canCastle(us, false))) {
        for (bool king_side : { true, false }) {
            if (board.castlingBlocked(us, king_side) && board.canCastle(us, king_side)) {
                encodeCastling(move_list, us, king_side);
            }
        }
    }
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

template void Board::generateMoves<GenType::NON_EVASIONS>(MoveList& move_list) const;
template void Board::generateMoves<GenType::CAPTURES>(MoveList& move_list) const;
template void Board::generateMoves<GenType::QUIET>(MoveList& move_list) const;
template void Board::generateMoves<GenType::EVASIONS>(MoveList& move_list) const;

template <>
void Board::generateMoves<GenType::LEGAL>(MoveList& move_list) const {
    // Generate legal moves
    Colour us = sideToMove;
    BitBoard pinned = blocks_for_king[static_cast<int>(us)] & piecesGet(us);
    Square ksq = squares(us == Colour::WHITE ? Pc_K : Pc_k);
    int idx = move_list._size();
    if (checkers()) {
        generateMoves<GenType::EVASIONS>(move_list);
    } else {
        generateMoves<GenType::NON_EVASIONS>(move_list);
    }

    while (idx != move_list._size()) {
        Square from = getFromSq(move_list[idx]);
        MoveCode code = getMoveCode(move_list[idx]);
        if (((pinned & mask(from)) || from == ksq || code == epcapture) && !isLegal(move_list[idx])) {
            move_list.removeMove(idx);
            // don't inc
        }
        else ++idx;
    }
}