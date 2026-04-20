#include "../include/BitBoards.hpp"
#include <vector>
#include <random>

BitBoard getOccupancy(int index, BitBoard mask) {
    BitBoard occupancy = 0ULL;
    int bits = popCount(mask);
    for (int i = 0; i < bits; i++) {
        int square = popLSB(mask);
        if (index & (1 << i)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

BitBoard slidingAttack(Square sq, BitBoard blockers, bool is_rook) {
    BitBoard attacks = 0ULL;
    
    struct Offset { int dr; int df; };
    std::vector<Offset> directions;

    if (is_rook) {
        directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; // N, S, E, W
    } else {
        directions = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // NE, NW, SE, SW
    }

    int start_rank = sq / 8;
    int start_file = sq % 8;

    for (auto d : directions) {
        int r = start_rank + d.dr;
        int f = start_file + d.df;

        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            BitBoard current_sqBB = (1ULL << (r * 8 + f));
            attacks |= current_sqBB;

            if (current_sqBB & blockers) {
                break;
            }

            r += d.dr;
            f += d.df;
        }
    }

    return attacks;
}

std::mt19937_64 rng(1234567); 

uint64_t random_uint64() {
    return rng();
}

uint64_t random_uint64_sparse() {
    return random_uint64() & random_uint64() & random_uint64();
}

uint64_t findMagic(Square sq, int bits, bool isRook) {
    BitBoard mask = isRook ? rook_masks[sq] : bishop_masks[sq];
    int variations = 1 << bits;
    
    std::vector<BitBoard> occupancies(variations);
    std::vector<BitBoard> attacks(variations);
    std::vector<BitBoard> used_attacks(variations);

    // Pre-calculate all possible occupancies and their real attacks
    for (int i = 0; i < variations; i++) {
        occupancies[i] = getOccupancy(i, mask);
        attacks[i] = slidingAttack(sq, occupancies[i], isRook);
    }

    while (true) {
        uint64_t magic = random_uint64_sparse();
        
        // Filter: Multiplications that don't reach high bits are useless
        if (popCount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        std::fill(used_attacks.begin(), used_attacks.end(), 0ULL);
        bool fail = false;

        for (int i = 0; i < variations; i++) {
            int idx = (occupancies[i] * magic) >> (64 - bits);
            if (used_attacks[idx] == 0ULL) {
                used_attacks[idx] = attacks[i]; // First time seeing this index
            } else if (used_attacks[idx] != attacks[i]) {
                fail = true; // Collision: two different attacks map to same index
                break;
            }
        }
        if (!fail) return magic;
    }
}

void initMagics() {
    BitBoard* rook_ptr = rook_table;
    BitBoard* bishop_ptr = bishop_table;

    for (Square sq = 0; sq < 64; ++sq) {
        // --- Setup Bishop ---
        BitBoard b_mask = bishop_masks[sq];
        int b_bits = popCount(b_mask);
        
        bishop_magics[sq].mask = b_mask;
        bishop_magics[sq].shift = 64 - b_bits;
        bishop_magics[sq].attacks = bishop_ptr;
        bishop_magics[sq].magic = findMagic(sq, b_bits, false);

        for (int i = 0; i < (1 << b_bits); i++) {
            BitBoard occ = getOccupancy(i, b_mask);
            int idx = (occ * bishop_magics[sq].magic) >> bishop_magics[sq].shift;
            bishop_magics[sq].attacks[idx] = slidingAttack(sq, occ, false);
        }
        bishop_ptr += (1ULL << b_bits);

        // --- Setup Rook ---
        BitBoard r_mask = rook_masks[sq];
        int r_bits = popCount(r_mask);
        
        rook_magics[sq].mask = r_mask;
        rook_magics[sq].shift = 64 - r_bits;
        rook_magics[sq].attacks = rook_ptr;
        rook_magics[sq].magic = findMagic(sq, r_bits, true);

        for (int i = 0; i < (1 << r_bits); i++) {
            BitBoard occ = getOccupancy(i, r_mask);
            int idx = (occ * rook_magics[sq].magic) >> rook_magics[sq].shift;
            rook_magics[sq].attacks[idx] = slidingAttack(sq, occ, true);
        }
        rook_ptr += (1ULL << r_bits);
    }
}