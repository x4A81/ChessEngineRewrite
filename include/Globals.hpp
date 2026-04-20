#pragma once
#include <cstdint>

using Piece = uint8_t;
using Square = uint8_t;
using Move = uint16_t;
using BitBoard = uint64_t;
inline constexpr Move nullmove = 0;

enum class Colour { BLACK, WHITE, NO_COLOUR };
enum class GenType { ALL, CAPTURES, QUIET, EVASIONS, LEGAL };
enum class Direction : int { North, South, East, West, NE, NW, SE, SW };

enum SquaresEncoding : Square {
    SQ_a1, SQ_b1, SQ_c1, SQ_d1, SQ_e1, SQ_f1, SQ_g1, SQ_h1,
    SQ_a2, SQ_b2, SQ_c2, SQ_d2, SQ_e2, SQ_f2, SQ_g2, SQ_h2,
    SQ_a3, SQ_b3, SQ_c3, SQ_d3, SQ_e3, SQ_f3, SQ_g3, SQ_h3,
    SQ_a4, SQ_b4, SQ_c4, SQ_d4, SQ_e4, SQ_f4, SQ_g4, SQ_h4,
    SQ_a5, SQ_b5, SQ_c5, SQ_d5, SQ_e5, SQ_f5, SQ_g5, SQ_h5,
    SQ_a6, SQ_b6, SQ_c6, SQ_d6, SQ_e6, SQ_f6, SQ_g6, SQ_h6,
    SQ_a7, SQ_b7, SQ_c7, SQ_d7, SQ_e7, SQ_f7, SQ_g7, SQ_h7,
    SQ_a8, SQ_b8, SQ_c8, SQ_d8, SQ_e8, SQ_f8, SQ_g8, SQ_h8, no_square
};

enum PiecesEncoding : Piece {
    p, n, b, r, q, k, P, N, B, R, Q, K, bpieces = 12, wpieces, allpieces, no_piece
};

/// Move encoding:
/// 6 bits for the from square
/// 6 bits for the to square
// 4 bits for a code
// code to     from
// 0000 000000 000000
// code  | Promo | capt | special 1 | sp 2 | type
/// 0    | 0     | 0    | 0         | 0    | quiet
/// 1    | 0     | 0    | 0         | 1    | dbp pawn
/// 2    | 0     | 0    | 1         | 0    | king castle
/// 3    | 0     | 0    | 1         | 1    | q castle
/// 4    | 0     | 1    | 0         | 0    | capture
/// 5    | 0     | 1    | 0         | 1    | ep capture
/// 8    | 1     | 0    | 0         | 0    | knight promo
/// 9    | 1     | 0    | 0         | 1    | bishop promo
/// 10   | 1     | 0    | 1         | 0    | rook promo
/// 11   | 1     | 0    | 1         | 1    | queen promo
/// 12   | 1     | 1    | 0         | 0    | knight promo capt
/// 13   | 1     | 1    | 0         | 1    | bishop promo capt
/// 14   | 1     | 1    | 1         | 0    | rook promo capt
/// 15   | 1     | 1    | 1         | 1    | queen promo capt

enum MoveCode : int {
    quiet, dblpush, kcastle, qcastle, capture, epcapture, 
    npromo = 8, bpromo, rpromo, qpromo, c_npromo, c_bpromo, c_rpromo, c_qpromo
};