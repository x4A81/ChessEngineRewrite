#pragma once
#include "Globals.hpp"
#include "BitMath.hpp"
#include <bit>
#include <array>
#include <print>

constexpr inline BitBoard rank1BB = 0xFFULL;
constexpr inline BitBoard rank2BB = rank1BB << 8;
constexpr inline BitBoard rank3BB = rank1BB << 16;
constexpr inline BitBoard rank4BB = rank1BB << 24;
constexpr inline BitBoard rank5BB = rank1BB << 32;
constexpr inline BitBoard rank6BB = rank1BB << 40;
constexpr inline BitBoard rank7BB = rank1BB << 48;
constexpr inline BitBoard rank8BB = rank1BB << 56;

inline void printBitBoard(BitBoard bb) {
    std::print("\n\n");
    for (int r = 7; r >= 0; --r) {
        std::print("  +---+---+---+---+---+---+---+---+\n");
        for (int f = 0; f < 8; ++f) {
            char piece = (bb & mask(8*r+f)) ? 'X' : ' ';
            if (f == 0) { std::print("%d ", r+1); }
            std::print("| %c ", piece);
        }
        std::print("| \n");
    }
    std::print("  +---+---+---+---+---+---+---+---+\n\n");
}

// functions to generate lookup tables
template <Colour us>
constexpr std::array<BitBoard, 64> generatePawn_att_table() {
    std::array<BitBoard, 64> table{};
    constexpr Direction upleft = (us == Colour::WHITE) ? Direction::NW : Direction::SE;
    constexpr Direction upright = (us == Colour::WHITE) ? Direction::NE : Direction::SW;
    for (int sq = 0; sq < 64; ++sq) {
        BitBoard bb = mask(sq);
        table[sq] = shift<upleft>(bb) | shift<upright>(bb);
    }

    return table;
}

alignas(64) constexpr inline std::array<BitBoard, 64> wpawn_att_table = generatePawn_att_table<Colour::WHITE>();
alignas(64) constexpr inline std::array<BitBoard, 64> bpawn_att_table = generatePawn_att_table<Colour::BLACK>();

constexpr std::array<BitBoard, 64> generateKnight_att_table() {
    std::array<BitBoard, 64> table{};
    BitBoard west, east, attacks;
    for (int sq = 0; sq < 64; ++sq) {
        BitBoard bb = mask(sq);
        east = shift<Direction::East>(bb);
        west = shift<Direction::West>(bb);
        attacks = (east|west) << 16;
        attacks |= (east|west) >> 16;
        east = shift<Direction::East>(east);
        west = shift<Direction::West>(west);
        attacks |= (east|west) <<  8;
        attacks |= (east|west) >>  8;
        
        table[sq] = attacks;
    }

    return table;
}

alignas(64) constexpr inline std::array<BitBoard, 64> knight_att_table = generateKnight_att_table();

constexpr std::array<BitBoard, 64> generateKing_att_table() {
    std::array<BitBoard, 64> table{};
    for (int sq = 0; sq < 64; ++sq) {
        BitBoard bb = mask(sq);
        BitBoard attacks = shift<Direction::East>(bb) | shift<Direction::West>(bb);
        bb |= attacks;
        attacks |= shift<Direction::North>(bb) | shift<Direction::South>(bb);
        table[sq] = attacks;
    }

    return table;
}

alignas(64) constexpr inline std::array<BitBoard, 64> king_att_table = generateKing_att_table();

constexpr std::array<int, 64> rook_shifts = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

constexpr std::array<int, 64> bishop_shifts = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};                     

// Function utilises lookup tables
template <Piece piece>
inline BitBoard attacks(Square sq) {
    if constexpr (piece == P) {
        
        return wpawn_att_table[sq];
    }
    else if constexpr (piece == p) {
        return bpawn_att_table[sq];
    }

    else if constexpr (piece == N || piece == n) {
        return knight_att_table[sq];
    }
    else if constexpr (piece == K || piece == k) {
        return king_att_table[sq];
    }

    return 0;
}