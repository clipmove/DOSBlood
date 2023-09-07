/*
 * Copyright (C) 2018, 2022 nukeykt
 *
 * This file is part of Blood-RE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DB_H_
#define _DB_H_

#include "build.h"

#define kMaxXSprites 2048
#define kMaxXWalls 512
#define kMaxXSectors 512

struct AISTATE;

struct XSPRITE {
    //int at0;
    signed   int reference : 14; // at0_0
    unsigned int at1_6 : 1;  // State 0
    unsigned int at1_7 : 17;
    unsigned int at4_0 : 10; // TX ID
    unsigned int at5_2 : 10; // RX ID
    unsigned int at6_4 : 8; // Cmd
    unsigned int at7_4 : 1; // going ON
    unsigned int at7_5 : 1; // going OFF
    unsigned int at7_6 : 2; // Wave
    unsigned int at8_0 : 12; // busyTime
    unsigned int at9_4 : 12; // waitTime
    unsigned int atb_0 : 1; // restState
    unsigned int atb_1 : 1; // Interruptable
    unsigned int atb_2 : 2; // unused
    unsigned int atb_4 : 2; // respawnPending
    unsigned int atb_6 : 1;
    unsigned int atb_7 : 1; // Launch Team
    unsigned int atc_0 : 8; // Drop Item
    unsigned int atd_0 : 1; // Decoupled
    unsigned int atd_1 : 1; // 1-shot
    unsigned int atd_2 : 1;
    unsigned int atd_3 : 3; // Key
    unsigned int atd_6 : 1; // Push
    unsigned int atd_7 : 1; // Vector
    unsigned int ate_0 : 1; // Impact
    unsigned int ate_1 : 1; // Pickup
    unsigned int ate_2 : 1; // Touch
    unsigned int ate_3 : 1; // Sight
    unsigned int ate_4 : 1; // Proximity
    unsigned int ate_5 : 2; // unused
    unsigned int ate_7 : 5; // Launch 12345
    unsigned int atf_4 : 1; // Single
    unsigned int atf_5 : 1; // Bloodbath
    unsigned int atf_6 : 1; // Coop
    unsigned int atf_7 : 1; // DudeLockout
    signed   int at10_0 : 16; // Data 1
    signed   int at12_0 : 16; // Data 2
    signed   int at14_0 : 16; // Data 3
    unsigned int at16_0 : 11;
    signed int at17_3 : 2; // Dodge
    unsigned int at17_5 : 1; // Locked
    unsigned int at17_6 : 2;
    unsigned int at18_0 : 2; // Respawn option
    unsigned int at18_2 : 16; // Data 4
    unsigned int at1a_2 : 6; // unused
    unsigned int at1b_0 : 8; // Lock msg
    unsigned int health : 12; // 1c_0
    unsigned int at1d_4 : 1; // dudeDeaf
    unsigned int at1d_5 : 1; // dudeAmbush
    unsigned int at1d_6 : 1; // dudeGuard
    unsigned int at1d_7 : 1; // DF reserved
    signed   int target : 16; // at1e target sprite
    signed   int at20_0 : 32; // target x
    signed   int at24_0 : 32; // target y
    signed   int at28_0 : 32;
    unsigned int at2c_0 : 16; // 2c_0
    signed   int at2e_0 : 16; // 2e_0
    unsigned int at30_0 : 16;
    unsigned int at32_0 : 16; // ai timer
    AISTATE *at34; // ai
}; // 56(0x38) bytes

struct XSECTOR {
    signed int reference : 14;
    unsigned int at1_6 : 1; // State 0
    unsigned int at1_7 : 17;
    unsigned int at4_0 : 16; // Data
    unsigned int at6_0 : 10; // TX ID
    unsigned int at7_2 : 3; // OFF->ON wave
    unsigned int at7_5 : 3; // ON->OFF wave
    unsigned int at8_0 : 10; // RX ID
    unsigned int at9_2 : 8; // Cmd 0
    unsigned int ata_2 : 1; // Send at ON
    unsigned int ata_3 : 1; // Send at OFF
    unsigned int ata_4 : 12; // OFF->ON busyTime
    unsigned int atc_0 : 12; // OFF->ON waitTime
    unsigned int atd_4 : 1;
    unsigned int atd_5 : 1; // Interruptable
    signed   int atd_6 : 8; // Lighting amplitude
    unsigned int ate_6 : 8; // Lighting freq
    unsigned int atf_6 : 1; // OFF->ON wait
    unsigned int atf_7 : 1; // ON->OFF wait
    unsigned int at10_0 : 8; // Lighting phase
    unsigned int at11_0 : 4; // Lighting wave
    unsigned int at11_4 : 1; // Lighting shadeAlways
    unsigned int at11_5 : 1; // Lighting floor
    unsigned int at11_6 : 1; // Lighting ceiling
    unsigned int at11_7 : 1; // Lighting walls
    signed   int at12_0 : 8; // Lighting value
    unsigned int at13_0 : 1; // Pan always
    unsigned int at13_1 : 1; // Pan floor
    unsigned int at13_2 : 1; // Pan ceiling
    unsigned int at13_3 : 1; // Pan drag
    unsigned int at13_4 : 1; // Underwater
    unsigned int at13_5 : 3; // Depth
    unsigned int at14_0 : 8; // Motion speed
    unsigned int at15_0 : 11; // Motion angle
    unsigned int at16_3 : 1;
    unsigned int at16_4 : 1; // Decoupled
    unsigned int at16_5 : 1; // 1-shot
    unsigned int at16_6 : 1;
    unsigned int at16_7 : 3; // Key
    unsigned int at17_2 : 1; // Push
    unsigned int at17_3 : 1; // Vector
    unsigned int at17_4 : 1; // Reserved
    unsigned int at17_5 : 1; // Enter
    unsigned int at17_6 : 1; // Exit
    unsigned int at17_7 : 1; // WallPush
    unsigned int at18_0 : 1; // Color Lights
    unsigned int at18_1 : 1;
    unsigned int at18_2 : 12; // ON->OFF busyTime
    unsigned int at19_6 : 12; // ON->OFF waitTime
    unsigned int at1b_2 : 1;
    unsigned int at1b_3 : 1;
    unsigned int at1b_4 : 4; // Ceil pal2
    signed   int at1c_0 : 32;
    signed   int at20_0 : 32;
    signed   int at24_0 : 32;
    signed   int at28_0 : 32;
    unsigned int at2c_0 : 16;
    unsigned int at2e_0 : 16;
    unsigned int at30_0 : 1; // Crush
    unsigned int at30_1 : 8; // Ceiling x panning frac
    unsigned int at31_1 : 8; // Ceiling y panning frac
    unsigned int at32_1 : 8; // Floor x panning frac
    unsigned int at33_1 : 3; // DamageType
    unsigned int at33_4 : 4; // Floor pal2
    unsigned int at34_0 : 8; // Floor y panning frac
    unsigned int at35_0 : 1; // Locked
    unsigned int at35_1 : 10; // Wind vel
    unsigned int at36_3 : 11; // Wind ang
    unsigned int at37_6 : 1; // Wind always
    unsigned int at37_7 : 1;
    unsigned int at38_0 : 11; // Motion Theta
    unsigned int at39_3 : 5; // Motion Z range
    signed   int at3a_0 : 12; // Motion speed
    unsigned int at3b_4 : 1; // Motion always
    unsigned int at3b_5 : 1; // Motion bob floor
    unsigned int at3b_6 : 1; // Motion bob ceiling
    unsigned int at3b_7 : 1; // Motion rotate
}; // 60(0x3c) bytes

struct XWALL {
    signed int reference : 14;
    unsigned int at1_6 : 1; // State
    unsigned int at1_7 : 17;
    signed int at4_0 : 16; // Data
    unsigned int at6_0 : 10; // TX ID
    unsigned int at7_2 : 6; // unused
    unsigned int at8_0 : 10; // RX ID
    unsigned int at9_2 : 8; // Cmd
    unsigned int ata_2 : 1; // going ON
    unsigned int ata_3 : 1; // going OFF
    unsigned int ata_4 : 12; // busyTime
    unsigned int atc_0 : 12; // waitTime
    unsigned int atd_4 : 1; // restState
    unsigned int atd_5 : 1; // Interruptable
    unsigned int atd_6 : 1; // panAlways
    signed   int atd_7 : 8; // panX
    signed   int ate_7 : 8; // panY
    unsigned int atf_7 : 1; // Decoupled
    unsigned int at10_0 : 1; // 1-shot
    unsigned int at10_1 : 1;
    unsigned int at10_2 : 3; // Key 
    unsigned int at10_5 : 1; // Push
    unsigned int at10_6 : 1; // Vector
    unsigned int at10_7 : 1; // Reserved
    unsigned int at11_0 : 2; // unused
    unsigned int at11_2 : 8; // x panning frac
    unsigned int at12_2 : 8; // y panning frac
    unsigned int at13_2 : 1; // Locked
    unsigned int at13_3 : 1; // DudeLockout
    unsigned int at13_4 : 4; // unused;
    unsigned int at14_0 : 32; // unused
}; // 24(0x18) bytes

enum {
    kStatRespawn = 8,
    kStatMarker = 10,
    kStatFree = 1024
};

enum {
    kMarker6 = 6,
    kMarker7,
    kMarkerWarpDest,
    kMarker9,
    kMarker10,
    kMarker13 = 13,
    kMarker14 = 14,
};

enum {
    kSpriteFlag0 = 1,
    kSpriteFlag1 = 2,
    kSpriteFlag2 = 4,
    kSpriteFlag3 = 8,
    kSpriteFlag4 = 16,
    kSpriteFlag5 = 32,
    kSpriteFlag8 = 256,
    kSpriteFlag9 = 512,
    kSpriteFlag10 = 1024,
    kSpriteFlag11 = 2048,
};

enum {
    kDudeBase = 200,
    kDudePlayer1 = 231,
    kDudePlayer8 = 238,
    kDudeMax = 254,

    kThingBase = 400,
    kThing415 = 415,
    kThing427 = 427,
    kThing428,
    kThingMax = 433,

    kMissileBase = 300,
    kMissile300 = kMissileBase,
    kMissile301,
    kMissile302,
    kMissile303,
    kMissile304,
    kMissile306 = 306,
    kMissileMax = 318,
};

enum {
    kSectorType604 = 604,
    kSectorType612 = 612,
    kSectorType617 = 617,
};

struct MAPHEADER2 {
    char at0[64];
    int at40; // xsprite size
    int at44; // xwall size
    int at48; // xsector size
    byte pad[52];
};

extern XSPRITE xsprite[kMaxXSprites];
extern XSECTOR xsector[kMaxXSectors];
extern XWALL xwall[kMaxXWalls];
extern long xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

extern long gVisibility;

extern ushort nextXSprite[kMaxXSprites];
extern ushort nextXWall[kMaxXWalls];
extern ushort nextXSector[kMaxXSectors];

extern BOOL char_1A76C6;
extern BOOL char_1A76C7;
extern BOOL char_1A76C8;

extern int gSongId;
extern int gMapRev;
extern int gSkyCount;

extern MAPHEADER2 char_19AE44;

extern char *gItemText[];
extern char *gAmmoText[];
extern char *gWeaponText[];

extern ushort gStatCount[kMaxStatus + 1];

int InsertSprite(int nSprite, int nStat);
void DeleteSprite(int nSprite);
void dbDeleteXSprite(int nXSprite);
int ChangeSpriteSect(int nSprite, int nSector);
int ChangeSpriteStat(int nSprite, int nStatus);
ushort dbInsertXSprite(int nSprite);

ulong dbReadMapCRC(char *pPath);

void dbLoadMap(char *pPath, long *pX, long *pY, long *pZ, short *pAngle, short *pSector, ulong *pCRC);

#endif
