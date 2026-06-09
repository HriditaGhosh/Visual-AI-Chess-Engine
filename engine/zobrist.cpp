#include "chess.h"

// ═══════════════════════════════════════════════════════════════
//  Zobrist key initialization
//
//  DSA: Bit Manipulation — LCG PRNG fills every key with a
//  distinct pseudo-random 64-bit number.  The seed is fixed so
//  the same hash always maps to the same position (reproducible).
// ═══════════════════════════════════════════════════════════════

ZobristKeys ZOBRIST;  // global singleton

uint64_t ZobristKeys::rand64(uint64_t& state) {
    // Splitmix64 — fast, high-quality 64-bit PRNG
    state += UINT64_C(0x9e3779b97f4a7c15);
    uint64_t z = state;
    // DSA: Bit Manipulation — mix bits via XOR-shifts and multiplications
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

ZobristKeys::ZobristKeys() {
    uint64_t state = UINT64_C(0xDEADBEEFCAFEBABE); // deterministic seed

    for (int c = 0; c < 2; ++c)
        for (int pt = 0; pt < 7; ++pt)
            for (int sq = 0; sq < 64; ++sq)
                pieces[c][pt][sq] = rand64(state);

    sideToMove = rand64(state);

    for (int c = 0; c < 2; ++c)
        for (int side = 0; side < 2; ++side)
            castling[c][side] = rand64(state);

    for (int f = 0; f < 8; ++f)
        enPassant[f] = rand64(state);
}
