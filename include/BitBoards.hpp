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

constexpr inline BitBoard fileABB = 0x0101010101010101ULL;
constexpr inline BitBoard fileBBB = fileABB << 1;
constexpr inline BitBoard fileCBB = fileABB << 2;
constexpr inline BitBoard fileDBB = fileABB << 3;
constexpr inline BitBoard fileEBB = fileABB << 4;
constexpr inline BitBoard fileFBB = fileABB << 5;
constexpr inline BitBoard fileGBB = fileABB << 6;
constexpr inline BitBoard fileHBB = fileABB << 7;

inline void printBB(BitBoard bb) {
    std::print("\n\n");
    for (int r = 7; r >= 0; --r) {
        std::print("  +---+---+---+---+---+---+---+---+\n");
        for (int f = 0; f < 8; ++f) {
            char piece = (bb & mask(8*r+f)) ? 'X' : ' ';
            if (f == 0) { std::printf("%d ", r+1); }
            std::printf("| %c ", piece);
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
    BitBoard west = 0ULL, east = 0ULL, attacks = 0ULL;
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

constexpr std::array<std::array<BitBoard, 64>, 64> generateBetweenBB() {
    std::array<std::array<BitBoard, 64>, 64> table{};
    for (int s1 = 0; s1 < 64; ++s1) {
        for (int s2 = 0; s2 < 64; ++s2) {

            table[s1][s2] = line(s1, s2);
        }
    }

    return table;
}

alignas(64) constexpr inline std::array<std::array<BitBoard, 64>, 64> betweenBB = generateBetweenBB();
constexpr inline BitBoard getBetweenBB(Square sq1, Square sq2) {
    return betweenBB[sq1][sq2];
}
// Fancy Magics

struct Magic {
    BitBoard* attacks;
    BitBoard mask;
    uint64_t magic;
    int shift;
};

alignas(64) inline BitBoard rook_table[0x19000];
alignas(64) inline BitBoard bishop_table[0x1480];

inline Magic rook_magics[64];
inline Magic bishop_magics[64];

constexpr std::array<BitBoard, 64> genRookMasks() {
    std::array<BitBoard, 64> masks{};
    for (Square sq = 0; sq < 64; ++sq) {
        BitBoard mask = 0ULL;
        mask |= ray<Direction::North>(sq) & ~rank8BB;
        mask |= ray<Direction::South>(sq) & ~rank1BB;
        mask |= ray<Direction::East>(sq) & ~fileHBB;
        mask |= ray<Direction::West>(sq) & ~fileABB;
        masks[sq] = mask;
    }

    return masks;
}

constexpr std::array<BitBoard, 64> genBishopMasks() {
    std::array<BitBoard, 64> masks{};
    constexpr BitBoard trim_edges = ~(rank1BB | rank8BB | fileABB | fileHBB);
    for (Square sq = 0; sq < 64; ++sq) {
        BitBoard mask = 0ULL;
        mask |= ray<Direction::NE>(sq) & trim_edges;
        mask |= ray<Direction::SE>(sq) & trim_edges;
        mask |= ray<Direction::SW>(sq) & trim_edges;
        mask |= ray<Direction::NW>(sq) & trim_edges;
        masks[sq] = mask;
    }

    return masks;
}

alignas(64) constexpr inline std::array<BitBoard, 64> rook_masks = genRookMasks();
alignas(64) constexpr inline std::array<BitBoard, 64> bishop_masks = genBishopMasks();

void initMagics();
BitBoard slidingAttack(Square sq, BitBoard blockers, bool is_rook);

inline BitBoard rookAttacks(Square sq, BitBoard blockers) {
    const Magic& m = rook_magics[sq];
    uint64_t index = ((blockers & m.mask) * m.magic) >> m.shift;
    return m.attacks[index];
}

inline BitBoard bishopAttacks(Square sq, BitBoard blockers) {
    const Magic& m = bishop_magics[sq];
    uint64_t index = ((blockers & m.mask) * m.magic) >> m.shift;
    return m.attacks[index];
}

// Function utilises lookup tables
template <Piece piece>
inline BitBoard attacks(Square sq) {
    assert(!(piece == Pc_B || piece == Pc_b || piece == Pc_R || piece == Pc_r || piece == Pc_Q || piece == Pc_q));
    if constexpr (piece == Pc_P)
        return wpawn_att_table[sq];
    else if constexpr (piece == Pc_p)
        return bpawn_att_table[sq];
    else if constexpr (piece == Pc_N || piece == Pc_n)
        return knight_att_table[sq];
    else if constexpr (piece == Pc_K || piece == Pc_k)
        return king_att_table[sq];
    
    else if constexpr (piece == Pc_K || piece == Pc_k)
        return king_att_table[sq];

    return 0;
}


// Function utilises lookup tables
template <Piece piece>
inline BitBoard attacks(Square sq, BitBoard blockers) {
    assert(!(piece == Pc_p || piece == Pc_P || piece == Pc_N || piece == Pc_n || piece == Pc_k || piece == Pc_K));
    if constexpr (piece == Pc_R || piece == Pc_r)
        return rookAttacks(sq, blockers);
    else if constexpr (piece == Pc_B || piece == Pc_b)
        return bishopAttacks(sq, blockers);
    else if constexpr (piece == Pc_Q || piece == Pc_q)
        return rookAttacks(sq, blockers) | bishopAttacks(sq, blockers);

    return 0;
}