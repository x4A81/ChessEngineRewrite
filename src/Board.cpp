#include "../include/Board.hpp"
#include "../include/BitMath.hpp"
#include "../include/BitBoards.hpp"
#include <sstream>
#include <print>
#include "Board.hpp"

Piece charToPiece(char c) {
    switch (c) {
        case 'p': return Pc_p;
        case 'n': return Pc_n;
        case 'b': return Pc_b;
        case 'r': return Pc_r;
        case 'q': return Pc_q;
        case 'k': return Pc_k;
        case 'P': return Pc_P;
        case 'N': return Pc_N;
        case 'B': return Pc_B;
        case 'R': return Pc_R;
        case 'Q': return Pc_Q;
        case 'K': return Pc_K;
        default:  return no_piece;
    }
}

Square stringToSquare(const string& sq) {
    int file = sq[0] - 'a';
    int rank = sq[1] - '1';
    return file + 8 * rank;
}

char pieceToChar(Piece piece) {
    std::string piece_chars = "pnbrqkPNBRQK";
    return piece_chars.at(piece);
}

void Board::reset() {
    std::fill(begin(bitBoards), end(bitBoards), 0ULL);
    std::fill(begin(pieceList), end(pieceList), no_piece);
    std::fill(begin(stateHistory), end(stateHistory), IrreversibleState{0, no_square, 0, no_piece});
    currentState.epSquare = no_square;
    currentState.castlingRights = 0;
    currentState.halfmoveClock = 0;
    sideToMove = Colour::NO_COLOUR;
    ply = 0;
}

bool Board::canCastle(Colour c, bool king_side) const {
    if (c == Colour::WHITE) {
        return (king_side && currentState.castlingRights & wking_side)
        || (!king_side && currentState.castlingRights & wqueen_side);
    } else {
            return (king_side && currentState.castlingRights & bking_side)
        || (!king_side && currentState.castlingRights & bqueen_side);
    }
}

bool Board::castlingBlocked(Colour c, bool king_side) const {
    CastlingRights cr;
    if (c == Colour::WHITE) {
        if (king_side) cr = wking_side;
        else cr = wqueen_side;
    } else {
        if (king_side) cr = bking_side;
        else cr = bqueen_side;
    }

    switch (cr) {
        case wking_side:
            return squares(SQ_f1) == no_piece && squares(SQ_g1) == no_piece;
        case wqueen_side:
            return squares(SQ_d1) == no_piece && squares(SQ_c1) == no_piece && squares(SQ_b1) == no_piece;

        case bking_side:
            return squares(SQ_f8) == no_piece && squares(SQ_g8) == no_piece;
        case bqueen_side:
            return squares(SQ_d8) == no_piece && squares(SQ_c8) == no_piece && squares(SQ_b8) == no_piece;

        default:
            break;
    }

    return false;
}

void Board::updateSliders(Colour c) {
    assert(c != Colour::NO_COLOUR);
    Square ksq = squares((c == Colour::WHITE) ? Pc_K : Pc_k);
    pinners[static_cast<int>(c)] = 0Ull;
    blocks_for_king[static_cast<int>(c)] = 0ULL;
    Colour them = oppC(c);
    // Snipers are sliders that attack the a square when all other pieces are removed
    BitBoard snipers = ((attacks<Pc_r>(ksq, 0ULL) & piecesOf(them, Pc_r))
                        | (attacks<Pc_b>(ksq, 0ULL) & piecesOf(them, Pc_b))
                        | (attacks<Pc_q>(ksq, 0ULL) & piecesOf(them, Pc_q)))
                        & ~piecesGet(them);
    BitBoard occ = piecesGet(allpieces) ^ snipers;
    while (snipers) {
        Square sn_sq = popLSB(snipers);
        BitBoard bb = getBetweenBB(ksq, sn_sq) & occ;
        if (bb & !moreThanOne(bb)) {
            blocks_for_king[static_cast<int>(c)] |= bb;
            if (bb & piecesGet(c))
                pinners[static_cast<int>(oppC(c))] |= bb;
        }
    }
}

void Board::loadFen(const string& fen) {
    reset();
    // Implementation for loading FEN string into board representation
    std::istringstream ss(fen);
    std::string board_part, stm, castling, enpassant, halfmove, fullmove;

    ss >> board_part >> stm >> castling >> enpassant >> halfmove >> fullmove;

    Square sq = 0;
    for (char c : board_part) {
        if (isdigit(c)) {
            sq += c - '0';
        } else if (c == '/') {
            continue;
        } else {
            Piece piece_idx = charToPiece(c);
            Square flip_sq = flipRank(sq);  
            pieceList[flip_sq] = piece_idx;
            setBit(pieces(piece_idx), flip_sq);
            sq++;
        }
    }

    sideToMove = (stm == "w") ? Colour::WHITE : Colour::BLACK;

    currentState.castlingRights = 0;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': currentState.castlingRights |= wking_side; break;
                case 'Q': currentState.castlingRights |= wqueen_side; break;
                case 'k': currentState.castlingRights |= bking_side; break;
                case 'q': currentState.castlingRights |= bqueen_side; break;
            }
        }
    }

    if (enpassant != "-") {
        currentState.epSquare = stringToSquare(enpassant);
    } else {
        currentState.epSquare = no_square;
    }

    bitBoards[bpieces] = bitBoards[Pc_p] | bitBoards[Pc_n] | bitBoards[Pc_b] |
                    bitBoards[Pc_r] | bitBoards[Pc_q] | bitBoards[Pc_k];
    bitBoards[wpieces] = bitBoards[Pc_P] | bitBoards[Pc_N] | bitBoards[Pc_B] |
                    bitBoards[Pc_R] | bitBoards[Pc_Q] | bitBoards[Pc_K];
    bitBoards[allpieces] = bitBoards[bpieces] | bitBoards[wpieces];
}

void Board::printBoard() const {
    for (int r = 7; r >= 0; --r) {
        std::print("+---+---+---+---+---+---+---+---+\n"); 
        for (int f = 0; f < 8; ++f) {
            char piece = ' ';
            if (pieceList[8*r+f] != no_piece)
                piece = pieceToChar(pieceList[8*r+f]);

            std::printf("| %c ", piece);
        }

        std::print("| \n");
    }
    std::print("+---+---+---+---+---+---+---+---+\n\n");
}

BitBoard Board::checkers() const {
    Square king_sq = sideToMove == Colour::WHITE ? squares(Pc_K) : squares(Pc_k);
    BitBoard attackers = 0ULL;
    if (sideToMove == Colour::WHITE) {
        attackers |= attacks<Pc_P>(king_sq) & bitBoards[Pc_p];
    }
    else 
        attackers |= attacks<Pc_p>(king_sq) & bitBoards[Pc_P];

    attackers |= attacks<Pc_N>(king_sq) & bitBoards[makePiece(Pc_n, oppC(sideToMove))];
    attackers |= attacks<Pc_B>(king_sq) & bitBoards[makePiece(Pc_b, oppC(sideToMove))];
    attackers |= attacks<Pc_R>(king_sq) & bitBoards[makePiece(Pc_r, oppC(sideToMove))];
    attackers |= attacks<Pc_Q>(king_sq) & bitBoards[makePiece(Pc_q, oppC(sideToMove))];
    return attackers;
}

void Board::makeMove(const Move move) {
    // Get move data
    const Square from_sq = getFromSq(move);
    const Square to_sq = getToSq(move);
    const Piece moving_piece = pieceList[from_sq];
    Piece captured_piece = pieceList[to_sq];
    const Colour piece_colour = (moving_piece >= Pc_P) ? Colour::WHITE : Colour::BLACK;
    const Square ep_square = currentState.epSquare;

    // Update board state
    stateHistory[ply] = currentState;
    ply++;
    currentState.epSquare = no_square;
    currentState.capturedPiece = captured_piece;
    if (isMove(move, epcapture)) {
        Square captured_sq = (piece_colour == Colour::WHITE) ? to_sq - 8 : to_sq + 8;
        captured_piece = pieceList[captured_sq];
    }
    currentState.halfmoveClock++;
    if (moving_piece == Pc_p || moving_piece == Pc_P || captured_piece != no_piece) {
        currentState.halfmoveClock = 0;
    }
    
    sideToMove = (sideToMove == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

    // Handle Casting rights
    currentState.castlingRights &= castlingRightsSqEncoder[from_sq];
    currentState.castlingRights &= castlingRightsSqEncoder[to_sq];

    if (isMove(move, quiet)) {
        // Normal move
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], to_sq);
    }

    else if (isMove(move, dblpush)) {
        // Normal move
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], to_sq);

        // Handle ep square
        if (piece_colour == Colour::WHITE) {
            currentState.epSquare = to_sq - 8;
        } else {
            currentState.epSquare = to_sq + 8;
        }
    }

    else if (isMove(move, capture)) {
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);
        popBit(bitBoards[captured_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        // Captured piece colour
        popBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
    }

    else if (isMove(move, epcapture)) {
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;

        // captured piece should have been updated for en passant above
        pieceList[ep_square] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);
        popBit(bitBoards[captured_piece], ep_square);
        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        // Captured piece colour
        popBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], ep_square);
        popBit(bitBoards[allpieces], from_sq);
        popBit(bitBoards[allpieces], ep_square);
        setBit(bitBoards[allpieces], to_sq);
    }

    else if (getMoveCode(move) >= npromo && getMoveCode(move) <= qpromo) {
        // Promotion move
        Piece promo_piece;
        switch (getMoveCode(move)) {
            case npromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_N : Pc_n; break;
            case bpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_B : Pc_b; break;
            case rpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_R : Pc_r; break;
            case qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_Q : Pc_q; break;
            default: promo_piece = no_piece; break;
        }

        pieceList[to_sq] = promo_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[promo_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], to_sq);
    }

    else if (getMoveCode(move) >= c_npromo) {
        // Promotion move
        Piece promo_piece;
        switch (getMoveCode(move)) {
            case c_npromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_N : Pc_n; break;
            case c_bpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_B : Pc_b; break;
            case c_rpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_R : Pc_r; break;
            case c_qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_Q : Pc_q; break;
            default: promo_piece = no_piece; break;
        }

        pieceList[to_sq] = promo_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[promo_piece], to_sq);
        popBit(bitBoards[captured_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
    }

    else if (isMove(move, kcastle)) {
        // King side castle
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], to_sq);

        // Move rook
        Square rook_from = (piece_colour == Colour::WHITE) ? SQ_h1 : SQ_h8;
        Square rook_to = (piece_colour == Colour::WHITE) ? SQ_f1 : SQ_f8;
        Piece rook_piece = pieceList[rook_from];
        pieceList[rook_to] = rook_piece;
        pieceList[rook_from] = no_piece;
        popBit(bitBoards[rook_piece], rook_from);
        setBit(bitBoards[rook_piece], rook_to);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_from);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_to);
        popBit(bitBoards[allpieces], rook_from);
        setBit(bitBoards[allpieces], rook_to);
    }

    else if (isMove(move, qcastle)) {
        // Queen side castle
        pieceList[to_sq] = moving_piece;
        pieceList[from_sq] = no_piece;
        popBit(bitBoards[moving_piece], from_sq);
        setBit(bitBoards[moving_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        popBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], to_sq);

        // Move rook
        Square rook_from = (piece_colour == Colour::WHITE) ? SQ_a1 : SQ_a8;
        Square rook_to = (piece_colour == Colour::WHITE) ? SQ_d1 : SQ_d8;
        Piece rook_piece = pieceList[rook_from];
        pieceList[rook_to] = rook_piece;
        pieceList[rook_from] = no_piece;
        popBit(bitBoards[rook_piece], rook_from);
        setBit(bitBoards[rook_piece], rook_to);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_from);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_to);
        popBit(bitBoards[allpieces], rook_from);
        setBit(bitBoards[allpieces], rook_to);
    }
}

void Board::unmakeMove(const Move move) {
    ply--;
    currentState = stateHistory[ply];
    sideToMove = (sideToMove == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

    // Get move data
    const Square from_sq = getFromSq(move);
    const Square to_sq = getToSq(move);
    const Piece moved_piece = pieceList[to_sq];
    const Piece captured_piece = currentState.capturedPiece;
    const Colour piece_colour = (moved_piece >= Pc_P) ? Colour::WHITE : Colour::BLACK;
    const Square ep_square = currentState.epSquare;

    if (isMove(move, quiet) || isMove(move, dblpush)) {
        // Normal move
        pieceList[to_sq] = no_piece;
        pieceList[from_sq] = moved_piece;
        popBit(bitBoards[moved_piece], to_sq);
        setBit(bitBoards[moved_piece], from_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        popBit(bitBoards[allpieces], to_sq);
        setBit(bitBoards[allpieces], from_sq);
    }

    else if (isMove(move, capture)) {
        pieceList[to_sq] = captured_piece;
        pieceList[from_sq] = moved_piece;
        popBit(bitBoards[moved_piece], to_sq);
        setBit(bitBoards[moved_piece], from_sq);
        setBit(bitBoards[captured_piece], to_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        // Captured piece colour
        setBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], to_sq);
        setBit(bitBoards[allpieces], from_sq);
    }

    else if (isMove(move, epcapture)) {
        pieceList[to_sq] = no_piece;
        pieceList[from_sq] = moved_piece;

        pieceList[ep_square] = captured_piece;
        popBit(bitBoards[moved_piece], to_sq);
        setBit(bitBoards[moved_piece], from_sq);
        popBit(bitBoards[captured_piece], ep_square);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        // Captured piece colour
        setBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], ep_square);
        setBit(bitBoards[allpieces], from_sq);
        setBit(bitBoards[allpieces], ep_square);
        popBit(bitBoards[allpieces], to_sq);
    }

    else if (getMoveCode(move) >= npromo) {
        // Promotion move
        Piece promo_piece;
        switch (getMoveCode(move)) {
            case c_npromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_N : Pc_n; break;
            case c_bpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_B : Pc_b; break;
            case c_rpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_R : Pc_r; break;
            case c_qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Pc_Q : Pc_q; break;
            default: promo_piece = no_piece; break;
        }

        pieceList[to_sq] = captured_piece;
        pieceList[from_sq] = moved_piece;
        setBit(bitBoards[moved_piece], from_sq);
        popBit(bitBoards[promo_piece], to_sq);
        
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        setBit(bitBoards[allpieces], from_sq);
        popBit(bitBoards[allpieces], to_sq);
        if (captured_piece != no_piece) {
            setBit(bitBoards[captured_piece], to_sq);
            setBit(bitBoards[piece_colour == Colour::BLACK ? wpieces : bpieces], to_sq);
            setBit(bitBoards[allpieces], to_sq);
        }
    }

    else if (isMove(move, kcastle)) {
        // King side castle
        pieceList[to_sq] = no_piece;
        pieceList[from_sq] = moved_piece;
        popBit(bitBoards[moved_piece], to_sq);
        setBit(bitBoards[moved_piece], from_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        popBit(bitBoards[allpieces], to_sq);
        setBit(bitBoards[allpieces], from_sq);

        // Move rook
        Square rook_from = (piece_colour == Colour::WHITE) ? SQ_h1 : SQ_h8;
        Square rook_to = (piece_colour == Colour::WHITE) ? SQ_f1 : SQ_f8;
        Piece rook_piece = pieceList[rook_to];
        pieceList[rook_to] = no_piece;
        pieceList[rook_from] = rook_piece;
        popBit(bitBoards[rook_piece], rook_to);
        setBit(bitBoards[rook_piece], rook_from);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_to);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_from);
        popBit(bitBoards[allpieces], rook_to);
        setBit(bitBoards[allpieces], rook_from);
    }

    else if (isMove(move, qcastle)) {
        // Queen side castle
        pieceList[to_sq] = no_piece;
        pieceList[from_sq] = moved_piece;
        popBit(bitBoards[moved_piece], to_sq);
        setBit(bitBoards[moved_piece], from_sq);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], to_sq);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], from_sq);
        popBit(bitBoards[allpieces], to_sq);
        setBit(bitBoards[allpieces], from_sq);

        // Move rook
        Square rook_from = (piece_colour == Colour::WHITE) ? SQ_a1 : SQ_a8;
        Square rook_to = (piece_colour == Colour::WHITE) ? SQ_d1 : SQ_d8;
        Piece rook_piece = pieceList[rook_to];
        pieceList[rook_to] = no_piece;
        pieceList[rook_from] = rook_piece;
        popBit(bitBoards[rook_piece], rook_to);
        setBit(bitBoards[rook_piece], rook_from);

        popBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_to);
        setBit(bitBoards[piece_colour == Colour::WHITE ? wpieces : bpieces], rook_from);
        popBit(bitBoards[allpieces], rook_to);
        setBit(bitBoards[allpieces], rook_from);
    }
}