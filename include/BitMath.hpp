#pragma once

#include "Globals.hpp"
#include <array>
#include <bit>
#include <cassert>

constexpr inline bool validSq(Square sq) {
    return sq >= 0 && sq < 64;
}

constexpr inline BitBoard mask(Square square) {
    assert(validSq(square));
    return 1ULL << square;
}

constexpr inline BitBoard flipRank(Square square) {
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

constexpr inline int popLSB(BitBoard& bb) {
    assert(bb != 0);
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    return sq;
}

constexpr inline int bitscanForward(BitBoard bb) {
    assert(bb != 0);
    return std::countr_zero(bb);
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
constexpr inline BitBoard shift(BitBoard bb) {
    int shift = shift_val[static_cast<int>(dir)];
    if (shift > 0)
        return (bb << shift) & noWraps[static_cast<int>(dir)];
    else
        return (bb >> -shift) & noWraps[static_cast<int>(dir)];
}