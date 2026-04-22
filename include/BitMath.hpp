#pragma once

#include "Globals.hpp"
#include <array>
#include <bit>
#include <cassert>

[[nodiscard]] constexpr inline bool validSq(Square sq) {
    return sq < 64;
}

[[nodiscard]] constexpr inline BitBoard mask(Square square) {
    assert(validSq(square));
    return 1ULL << square;
}

[[nodiscard]] constexpr inline BitBoard flipRank(Square square) {
    assert(validSq(square));
    return square ^ 56;
}

constexpr inline void popBit(BitBoard& bb, Square square) {
    assert(validSq(square));
    bb &= ~mask(square);
}

constexpr inline void setBit(BitBoard& bb, Square square) {
    assert(validSq(square));
    bb |= mask(square);
}

[[nodiscard]] constexpr inline Square popLSB(BitBoard& bb) {
    assert(bb != 0);
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    return sq;
}

[[nodiscard]] constexpr inline Square lsb(BitBoard bb) {
    assert(bb != 0);
    return std::countr_zero(bb);
}

[[nodiscard]] constexpr inline Square popCount(BitBoard bb) {
    return std::popcount(bb);
}

// Designed to be indexed by Direction enum
constexpr inline std::array<BitBoard, 8> noWraps = {
    0xFFFFFFFFFFFFFF00, 0x00FFFFFFFFFFFFFF, 0xFEFEFEFEFEFEFEFE, 0x7F7F7F7F7F7F7F7F, 
    0xFEFEFEFEFEFEFE00, 0x7F7F7F7F7F7F7F00, 0x00FEFEFEFEFEFEFE, 0x007F7F7F7F7F7F7F 
};
constexpr inline std::array<int, 8> shift_val = {
    8, -8, 1, -1, 9, 7, -7, -9
};

template <Direction dir>
[[nodiscard]] constexpr inline BitBoard shift(BitBoard bb) {
    int shift = shift_val[static_cast<int>(dir)];
    if (shift > 0)
        return (bb << shift) & noWraps[static_cast<int>(dir)];
    else
        return (bb >> -shift) & noWraps[static_cast<int>(dir)];
}

template <Direction dir>
[[nodiscard]] constexpr inline BitBoard ray(Square sq) {
    assert(validSq(sq));
    BitBoard bb = mask(sq);
    BitBoard result = 0ULL;
    while ((bb = shift<dir>(bb))) {
        result |= bb;
    }
    return result;
}

// Returns a ray from s1 to s2, excluding s1, including s2
[[nodiscard]] constexpr inline BitBoard line(Square s1, Square s2) {
    assert(validSq(s1) && validSq(s2));
    if (ray<Direction::North>(s1) & mask(s2)) 
        return ray<Direction::North>(s1) & ~ray<Direction::North>(s2);
        
    if (ray<Direction::South>(s1) & mask(s2)) 
        return ray<Direction::South>(s1) & ~ray<Direction::South>(s2);
        
    if (ray<Direction::East>(s1) & mask(s2)) 
        return ray<Direction::East>(s1) & ~ray<Direction::East>(s2);
        
    if (ray<Direction::West>(s1) & mask(s2)) 
        return ray<Direction::West>(s1) & ~ray<Direction::West>(s2);
        
    if (ray<Direction::NE>(s1) & mask(s2)) 
        return ray<Direction::NE>(s1) & ~ray<Direction::NE>(s2);
        
    if (ray<Direction::NW>(s1) & mask(s2)) 
        return ray<Direction::NW>(s1) & ~ray<Direction::NW>(s2);
        
    if (ray<Direction::SE>(s1) & mask(s2)) 
        return ray<Direction::SE>(s1) & ~ray<Direction::SE>(s2);
        
    if (ray<Direction::SW>(s1) & mask(s2)) 
        return ray<Direction::SW>(s1) & ~ray<Direction::SW>(s2);
    return 0ULL;
}