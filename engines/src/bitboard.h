#ifndef _BITBOARD_H_
#define _BITBOARD_H_

#include "oxchess.h"

// Attack tables

extern U64 kingAttackTable[64];
extern U64 knightAttackTable[64];
extern U64 pawnAttackTable[2][64];

extern U64 bishopAttackTable[64][512]; 
extern U64 rookAttackTable[64][4096];

#define aFile C64(0x0101010101010101)
#define hFile C64(0x8080808080808080)
#define abFile C64(0x0303030303030303)
#define ghFile C64(0xC0C0C0C0C0C0C0C0)

#define north_one(bb) ((bb) << 8)
#define south_one(bb) ((bb) >> 8)
#define west_one(bb) (((bb) >> 1) & ~hFile)
#define east_one(bb) (((bb) << 1) & ~aFile)

#define north_two(bb) ((bb) << 16)
#define south_two(bb) ((bb) >> 16)
#define west_two(bb) (((bb) >> 2) & ~ghFile)
#define east_two(bb) (((bb) << 2) & ~abFile)

// Magic bitboards

typedef struct {
    U64 mask;
    U64 magic;
} MagicInfo;

void print_bitboard(U64);

U64 bishopAttacks(int, U64);

U64 rookAttacks(int, U64);

void initAttackTables();

#endif