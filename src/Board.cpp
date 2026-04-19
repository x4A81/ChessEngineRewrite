#include "../include/Board.hpp"
#include "../include/BitMath.hpp"
#include <sstream>
#include <print>

Piece charToPiece(char c) {
    switch (c) {
        case 'p': return p;
        case 'n': return n;
        case 'b': return b;
        case 'r': return r;
        case 'q': return q;
        case 'k': return k;
        case 'P': return P;
        case 'N': return N;
        case 'B': return B;
        case 'R': return R;
        case 'Q': return Q;
        case 'K': return K;
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
            bitBoards[piece_idx] |= mask(flip_sq);
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

    bitBoards[bpieces] = bitBoards[p] | bitBoards[n] | bitBoards[b] |
                    bitBoards[r] | bitBoards[q] | bitBoards[k];
    bitBoards[wpieces] = bitBoards[P] | bitBoards[N] | bitBoards[B] |
                    bitBoards[R] | bitBoards[Q] | bitBoards[K];
    bitBoards[allpieces] = bitBoards[bpieces] | bitBoards[wpieces];
}

void Board::printBoard() {
    for (int r = 7; r >= 0; --r) {
        std::print("+---+---+---+---+---+---+---+---+\n"); 
        for (int f = 0; f < 8; ++f) {
            char piece = ' ';
            if (pieceList[8*r+f] != no_piece)
                piece = pieceToChar(pieceList[8*r+f]);

            std::print("| %c ", piece);
        }

        std::print("| \n");
    }
    std::print("+---+---+---+---+---+---+---+---+\n\n");
}

void Board::makeMove(Move move) {
    // Get move data
    const Square from_sq = getFromSq(move);
    const Square to_sq = getToSq(move);
    const Piece moving_piece = pieceList[from_sq];
    Piece captured_piece = pieceList[to_sq];
    const Colour piece_colour = (moving_piece >= P) ? Colour::WHITE : Colour::BLACK;
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
    if (moving_piece == p || moving_piece == P || captured_piece != no_piece) {
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
            case npromo: promo_piece = (piece_colour == Colour::WHITE) ? N : n; break;
            case bpromo: promo_piece = (piece_colour == Colour::WHITE) ? B : b; break;
            case rpromo: promo_piece = (piece_colour == Colour::WHITE) ? R : r; break;
            case qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Q : q; break;
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
            case npromo: promo_piece = (piece_colour == Colour::WHITE) ? N : n; break;
            case bpromo: promo_piece = (piece_colour == Colour::WHITE) ? B : b; break;
            case rpromo: promo_piece = (piece_colour == Colour::WHITE) ? R : r; break;
            case qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Q : q; break;
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

void Board::unmakeMove(Move move) {
    ply--;
    currentState = stateHistory[ply];
    sideToMove = (sideToMove == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

    // Get move data
    const Square from_sq = getFromSq(move);
    const Square to_sq = getToSq(move);
    const Piece moved_piece = pieceList[to_sq];
    const Piece captured_piece = currentState.capturedPiece;
    const Colour piece_colour = (moved_piece >= P) ? Colour::WHITE : Colour::BLACK;
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
            case npromo: promo_piece = (piece_colour == Colour::WHITE) ? N : n; break;
            case bpromo: promo_piece = (piece_colour == Colour::WHITE) ? B : b; break;
            case rpromo: promo_piece = (piece_colour == Colour::WHITE) ? R : r; break;
            case qpromo: promo_piece = (piece_colour == Colour::WHITE) ? Q : q; break;
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