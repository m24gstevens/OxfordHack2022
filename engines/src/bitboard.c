#include "bitboard.h"

void print_bitboard(U64 bb) {
    int i,j;
    for (j=7;j>=0;j--) {
        printf(" %d. ",j);
        for (i=0; i<8; i++) {
            printf("%d ", (TEST_BIT(bb,8*j+i) ? 1 : 0));
        }
        printf("\n");
    }
    printf("   A B C D E F G H\n");
}

// Jumper pieces

U64 kingAttackTable[64];
U64 knightAttackTable[64];
U64 pawnAttackTable[2][64];

U64 knightAttacks(U64 bb) {
    U64 attacks = C64(0);
    U64 mask = east_one(bb) | west_one(bb);
    attacks |= north_two(mask) | south_two(mask);
    mask = east_two(bb) | west_two(bb);
    attacks |= north_one(mask) | south_one(mask);

    return attacks;
}

void initKnightTable() {
    U64 bb;
    for (int i=0; i<64; i++) {
        bb = C64(1) << i;
        knightAttackTable[i] = knightAttacks(bb);
    }
}

U64 kingAttacks(U64 bb) {
    U64 attacks = east_one(bb) | west_one(bb);
    attacks |= north_one(attacks) | south_one(attacks);
    attacks |= north_one(bb) | south_one(bb);

    return attacks;
}

void initKingTable() {
    U64 bb;
    for (int i=0; i<64; i++) {
        bb = C64(1) << i;
        kingAttackTable[i] = kingAttacks(bb);
    }
}

U64 pawnAttacks(U64 bb, int side) {
    U64 mask = east_one(bb) | west_one(bb);

    return (side ? south_one(mask) : north_one(mask));
}

void initPawnTable() {
    U64 bb;
    for (int s=0; s<2; s++) {
        for (int i=0; i<64; i++) {
            bb = C64(1) << i;
            pawnAttackTable[s][i] = pawnAttacks(bb,s);
        }
    }
}

void initJumperTables() {
    initKnightTable();
    initKingTable();
    initPawnTable();
}


// Sliding pieces

U64 bishopAttackTable[64][512]; 
U64 rookAttackTable[64][4096];

MagicInfo magicBishopInfo[64];
MagicInfo magicRookInfo[64];

// Number of bits needed for magic bitboards

const int rookBits[64] = {
    12,11,11,11,11,11,11,12,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    12,11,11,11,11,11,11,12
};

const int bishopBits[64] = {
    6,5,5,5,5,5,5,6,
    5,5,5,5,5,5,5,5,
    5,5,7,7,7,7,5,5,
    5,5,7,9,9,7,5,5,
    5,5,7,9,9,7,5,5,
    5,5,7,7,7,7,5,5,
    5,5,5,5,5,5,5,5,
    6,5,5,5,5,5,5,6
};

// Magic numbers

const U64 rookMagic[64] = {
  C64(0xa800080c0001020),
  C64(0x1480200080400010),
  C64(0x9100200008104101),
  C64(0x100041000082100),
  C64(0x2200020010200804),
  C64(0x200010804100200),
  C64(0x41000100020000c4),
  C64(0x300160040822300),
  C64(0x2800280604000),
  C64(0x80c00040201000),
  C64(0x40802000801000),
  C64(0x2004008220010),
  C64(0x402800400080082),
  C64(0x806000200100408),
  C64(0x2004001102241038),
  C64(0x8641000082004100),
  C64(0x288004400880),
  C64(0x8106848040022000),
  C64(0x1060008010002080),
  C64(0x1001010020081000),
  C64(0x8010004891100),
  C64(0x6808002008400),
  C64(0x4040008114290),
  C64(0x20004006081),
  C64(0x400080208008),
  C64(0x1280400040201000),
  C64(0x3020200180100080),
  C64(0x410080080100084),
  C64(0x50040080080080),
  C64(0x4020080800400),
  C64(0x4001006100020024),
  C64(0x200140200019a51),
  C64(0x88804010800420),
  C64(0x8810004000c02000),
  C64(0x8a008022004013),
  C64(0x4008008208801000),
  C64(0x4008002004040040),
  C64(0xd244800200800400),
  C64(0x12a100114005882),
  C64(0x8008044020000a1),
  C64(0x8840058004458020),
  C64(0x90002000d0014000),
  C64(0x420001008004040),
  C64(0x8080080010008080),
  C64(0x1006001008220004),
  C64(0x408841020080140),
  C64(0x10080210040001),
  C64(0x8002040060820001),
  C64(0x40810a002600),
  C64(0x342a0100844200),
  C64(0x840110020084100),
  C64(0x4208001000088080),
  C64(0x10c041008010100),
  C64(0x82001024085200),
  C64(0x800800100020080),
  C64(0x2000010850840200),
  C64(0x80002018408301),
  C64(0x82402210820506),
  C64(0x82200801042),
  C64(0x9220042008100101),
  C64(0x342000820041102),
  C64(0x402008104502802),
  C64(0x441001200088421),
  C64(0x42401288c090042),
};

const U64 bishopMagic[64] = {
  C64(0x840a20802008011),
  C64(0x802100400908001),
  C64(0x12141042008249),
  C64(0x8204848004882),
  C64(0xd00510c004808246),
  C64(0x4022020320000010),
  C64(0x4050880882100800),
  C64(0x4802402208200401),
  C64(0x100204810808090),
  C64(0xc400100242004200),
  C64(0x1820c20400408000),
  C64(0x282080202000),
  C64(0x20210000c00),
  C64(0x2001488824401020),
  C64(0x209008401201004),
  C64(0x20010846322012),
  C64(0xc040111090018100),
  C64(0x4001004a09406),
  C64(0x8004405001020014),
  C64(0x8001c12102020),
  C64(0x1001000290400200),
  C64(0x841001820884000),
  C64(0x88c010124010440),
  C64(0x410a0000211c0200),
  C64(0x4c04402010020820),
  C64(0x2288110010800),
  C64(0x2084040080a0040),
  C64(0x30020020080480e0),
  C64(0x84040004410040),
  C64(0xc000888001082000),
  C64(0x4088208048a1004),
  C64(0x14004014222200),
  C64(0x30a200410305060),
  C64(0x804010806208200),
  C64(0x82002400120800),
  C64(0x200020082180081),
  C64(0x8240090100001040),
  C64(0xd200a100420041),
  C64(0x9c50040048010140),
  C64(0x8002004a0b084200),
  C64(0x8220a0105140),
  C64(0x14210442001030),
  C64(0x2140124000800),
  C64(0x1008102018000908),
  C64(0x48d0a000400),
  C64(0x1013003020081),
  C64(0x2020204183210),
  C64(0x2020204220200),
  C64(0x1080202620002),
  C64(0x2202609103800c1),
  C64(0x50183020841040a0),
  C64(0x800108246080020),
  C64(0x10084010248000),
  C64(0x40942480200c2),
  C64(0x4200842008812),
  C64(0x10010824888800),
  C64(0x1010080a00800),
  C64(0x608008400825000),
  C64(0x8021540040441000),
  C64(0x2022484800840400),
  C64(0x100000040810b400),
  C64(0x90044500a0208),
  C64(0x94010520a0041),
  C64(0x40110141030100),
};

// Functions

static inline U64 indexToU64(int idx, int num_bits, U64 mask) {
    U64 occupancy = C64(0);
    for (int count=0; count < num_bits; count++) {
        int lsb = Bitscan(mask);
        POP_LS1B(mask);
        if (idx & (1 << count)) {
            occupancy |= (C64(1) << lsb);
        }
    }
    return occupancy;
}

static inline U64 bishopOccupancyMask(int i) {
    int r,f,rank,file;
    rank = RANK(i);
    file = FILE(i);

    U64 mask = C64(0);
    for (r=rank+1, f=file+1; r <= 6 && f <= 6; r++, f++) mask |= ((U64)1 << (f + 8*r));
    for (r=rank+1, f=file-1; r <= 6 && f >= 1; r++, f--) mask |= ((U64)1 << (f + 8*r));
    for (r=rank-1, f=file+1; r >= 1 && f <= 6; r--, f++) mask |= ((U64)1 << (f + 8*r));
    for (r=rank-1, f=file-1; r >= 1 && f >= 1; r--, f--) mask |= ((U64)1 << (f + 8*r));

    return mask;
}

static inline U64 rookOccupancyMask(int i) {
    int r,f,rank,file;
    rank = RANK(i);
    file = FILE(i);

    U64 mask = C64(0);
    for (r = rank+1; r<=6; r++) mask |= (C64(1) << (file + 8*r));
    for (r = rank-1; r>=1; r--) mask |= (C64(1) << (file + 8*r));
    for (f = file+1; f<=6; f++) mask |= (C64(1) << (f + 8*rank));
    for (f = file-1; f>=1; f--) mask |= (C64(1) << (f + 8*rank));

    return mask;
}

static inline U64 bishopAttackMask(int sq, U64 block) {
    int r,f,rank,file;
    rank = RANK(sq);
    file = FILE(sq);
    U64 attack = C64(0);
    for (r=rank+1, f=file+1; r <= 7 && f <= 7; r++, f++) {
        attack |= (C64(1) << (f + 8*r));
        if (block & (C64(1) << (f + 8*r))) break;
    }
    for (r=rank+1, f=file-1; r <= 7 && f >= 0; r++, f--) {
        attack |= (C64(1) << (f + 8*r));
        if (block & (C64(1) << (f + 8*r))) break;
    }
    for (r=rank-1, f=file+1; r >= 0 && f <= 7; r--, f++) {
        attack |= (C64(1) << (f + 8*r));
        if (block & (C64(1) << (f + 8*r))) break;
    }
    for (r=rank-1, f=file-1; r >= 0 && f >= 0; r--, f--) {
        attack |= (C64(1) << (f + 8*r));
        if (block & (C64(1) << (f + 8*r))) break;
    }
    return attack;
}

static inline U64 rookAttackMask(int sq, U64 block) {
    int r,f,rank,file;
    rank = RANK(sq);
    file = FILE(sq);
    U64 attack = C64(0);
    for (r=rank+1; r <= 7; r++) {
        attack |= (C64(1) << (file + 8*r));
        if (block & (C64(1) << (file + 8*r))) break;
    }
    for (r=rank-1; r >=0; r--) {
        attack |= (C64(1) << (file + 8*r));
        if (block & (C64(1) << (file + 8*r))) break;
    }
    for (f=file+1; f <= 7; f++) {
        attack |= (C64(1) << (f + 8*rank));
        if (block & (C64(1) << (f + 8*rank))) break;
    }
    for (f=file-1; f >= 0; f--) {
        attack |= (C64(1) << (f + 8*rank));
        if (block & (C64(1) << (f + 8*rank))) break;
    }
    return attack;
}

void initBishopMagic() {
    U64 occupancy, occ, mask;
    int shift;
    for (int i=0; i<64; i++) {
        MagicInfo info = {.mask = bishopOccupancyMask(i), .magic = bishopMagic[i]};
        magicBishopInfo[i] = info;
    }

    for (int i=0; i<64; i++) {
        shift = bishopBits[i];
        occupancy = bishopOccupancyMask(i);
        for (int j=0; j<(1<<shift); j++) {
            occ = indexToU64(j,shift,occupancy);
            mask = occ * bishopMagic[i];
            mask >>= (64 - shift);
            bishopAttackTable[i][mask] = bishopAttackMask(i,occ); 
        }
    }
}

void initRookMagic() {
    U64 occupancy, occ, mask;
    int shift;
    for (int i=0; i<64; i++) {
        MagicInfo info = {.mask = rookOccupancyMask(i), .magic = rookMagic[i]};
        magicRookInfo[i] = info;
    }

    for (int i=0; i<64; i++) {
        shift = rookBits[i];
        occupancy = rookOccupancyMask(i);
        for (int j=0; j<(1<<shift); j++) {
            occ = indexToU64(j,shift,occupancy);
            mask = occ * rookMagic[i];
            mask >>= (64 - shift);
            rookAttackTable[i][mask] = rookAttackMask(i,occ); 
        }
    }
}

U64 bishopAttacks(int sq, U64 occ) {
    occ &= magicBishopInfo[sq].mask;
    occ *= magicBishopInfo[sq].magic;
    occ >>= (64 - bishopBits[sq]);
    return bishopAttackTable[sq][occ];
}

U64 rookAttacks(int sq, U64 occ) {
    occ &= magicRookInfo[sq].mask;
    occ *= magicRookInfo[sq].magic;
    occ >>= (64 - rookBits[sq]);
    return rookAttackTable[sq][occ];
}


void initSliderTables() {
    initBishopMagic();
    initRookMagic();
}

void initAttackTables() {
    initJumperTables();
    initSliderTables();
}