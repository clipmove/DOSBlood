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
#include <string.h>
#include <stdio.h>
#include "typedefs.h"
#include "actor.h"
#include "build.h"
#include "config.h"
#include "db.h"
#include "debug4g.h"
#include "demo.h"
#include "dude.h"
#include "error.h"
#include "eventq.h"
#include "fx.h"
#include "gameutil.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "map2d.h"
#include "network.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "tile.h"
#include "trig.h"
#include "triggers.h"
#include "warp.h"
#include "weapon.h"
#include "view.h"

static void PlayerSurvive(int, int);
static void PlayerKeelsOver(int, int);

static int nPlayerSurviveClient = seqRegisterClient(PlayerSurvive);
static int nPlayerKeelClient = seqRegisterClient(PlayerKeelsOver);

PROFILE gProfile[kMaxPlayers];

PLAYER gPlayer[kMaxPlayers];

PLAYER *gMe, *gView;

int int_21EFB0[8];
int int_21EFD0[8];

struct ARMORDATA {
    int at0;
    int at4;
    int at8;
    int atc;
    int at10;
    int at14;
};
ARMORDATA armorData[5] = {
    { 0x320, 0x640, 0x320, 0x640, 0x320, 0x640 },
    { 0x640, 0x640, 0, 0x640, 0, 0x640 },
    { 0, 0x640, 0x640, 0x640, 0, 0x640 },
    { 0, 0x640, 0, 0x640, 0x640, 0x640 },
    { 0xc80, 0xc80, 0xc80, 0xc80, 0xc80, 0xc80 }
};

int Handicap[] = {
    144, 208, 256, 304, 368
};

struct VICTORY {
    char *at0;
    int at4;
};

VICTORY gVictory[] = {
    { "%s boned %s like a fish", 4100 },
    { "%s castrated %s", 4101 },
    { "%s creamed %s", 4102 },
    { "%s destroyed %s", 4103 },
    { "%s diced %s", 4104 },
    { "%s disemboweled %s", 4105 },
    { "%s flattened %s", 4106 },
    { "%s gave %s Anal Justice", 4107 },
    { "%s gave AnAl MaDnEsS to %s", 4108 },
    { "%s hurt %s real bad", 4109 },
    { "%s killed %s", 4110 },
    { "%s made mincemeat out of %s", 4111 },
    { "%s massacred %s", 4112 },
    { "%s mutilated %s", 4113 },
    { "%s reamed %s", 4114 },
    { "%s ripped %s a new orifice", 4115 },
    { "%s slaughtered %s", 4116 },
    { "%s sliced %s", 4117 },
    { "%s smashed %s", 4118 },
    { "%s sodomized %s", 4119 },
    { "%s splattered %s", 4120 },
    { "%s squashed %s", 4121 },
    { "%s throttled %s", 4122 },
    { "%s wasted %s", 4123 },
    { "%s body bagged %s", 4124 },
};

struct SUICIDE {
    char *at0;
    int at4;
};

SUICIDE gSuicide[] = {
    { "%s is excrement", 4202 },
    { "%s is hamburger", 4203 },
    { "%s suffered scrotum separation", 4204 },
    { "%s volunteered for population control", 4206 },
    { "%s has suicided", 4207 },
};

POSTURE gPosture[2][3] = {
    {
        {
            0x4000,
            0x4000,
            0x4000,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1600,
            0x1200,
            0xc00,
            0x90
        },
        {
            0x1200,
            0x1200,
            0x1200,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1400,
            0x1000,
            -0x600,
            0xb0
        },
        {
            0x2000,
            0x2000,
            0x2000,
            22,
            28,
            24,
            16,
            16,
            40,
            0x800,
            0x600,
            -0x600,
            0xb0
        },
    },
    {
        {
            0x4000,
            0x4000,
            0x4000,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1600,
            0x1200,
            0xc00,
            0x90
        },
        {
            0x1200,
            0x1200,
            0x1200,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1400,
            0x1000,
            -0x600,
            0xb0
        },
        {
            0x2000,
            0x2000,
            0x2000,
            22,
            28,
            24,
            16,
            16,
            40,
            0x800,
            0x600,
            -0x600,
            0xb0
        },
    }
};

AMMOINFO gAmmoInfo[] = {
    { 0, -1 },
    { 100, -1 },
    { 100, 4 },
    { 500, 5 },
    { 100, -1 },
    { 50, -1 },
    { 2880, -1 },
    { 250, -1 },
    { 100, -1 },
    { 100, -1 },
    { 50, -1 },
    { 50, -1 },
};

POWERUPINFO gPowerUpInfo[kMaxPowerUps] = {
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 0, 100, 100 },
    { -1, 0, 50, 100 },
    { -1, 0, 20, 100 },
    { -1, 0, 100, 200 },
    { -1, 0, 2, 200 },
    { 783, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 1, 3600, 432000 },
    { 827, 0, 3600, 432000 },
    { 828, 0, 3600, 432000 },
    { -1, 0, 3600, 1728000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { 851, 0, 3600, 432000 },
    { 2428, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 900, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 1, 3600, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }
};

struct DAMAGEINFO {
    int at0;
    int at4[3];
    int at10[3];
};

DAMAGEINFO damageInfo[7] = {
    { -1, 731, 732, 733, 710, 710, 710 },
    { 1, 742, 743, 744, 711, 711, 711 },
    { 0, 731, 732, 733, 712, 712, 712 },
    { 1, 731, 732, 733, 713, 713, 713 },
    { -1, 724, 724, 724, 714, 714, 714 },
    { 2, 731, 732, 733, 715, 715, 715 },
    { 0, 0, 0, 0, 0, 0, 0 }
};

static int powerupToPackItem(int);

int powerupCheck(PLAYER *pPlayer, int nPowerUp)
{
    dassert(pPlayer != NULL, 403);
    dassert(nPowerUp >= 0 && nPowerUp < kMaxPowerUps, 404);
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack < 0)
        return pPlayer->at202[nPowerUp];
    if (!packItemActive(pPlayer, nPack))
    {
        return 0;
    }
    return pPlayer->at202[nPowerUp];
}

static BOOL powerupAkimboWeapons(int nWeapon)
{
    switch (nWeapon)
    {
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
        return 1;
    default:
        break;
    }
    return 0;
}

BOOL powerupActivate(PLAYER *pPlayer, int nPowerUp)
{
    if (powerupCheck(pPlayer, nPowerUp) > 0 && gPowerUpInfo[nPowerUp].at2)
        return 0;
    if (!pPlayer->at202[nPowerUp])
        pPlayer->at202[nPowerUp] = gPowerUpInfo[nPowerUp].at3;
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0)
        pPlayer->packInfo[nPack].at0 = 1;
    switch (nPowerUp+100)
    {
    case 112:
    case 115: // jump boots
        pPlayer->ata1[0]++;
        break;
    case 124: // reflective shots
        if (pPlayer == gMe && gGameOptions.nGameType == GAMETYPE_0)
            sfxSetReverb2(1);
        break;
    case 114: // death mask
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]++;
        break;
    }
    case 118:
        pPlayer->ata1[4]++;
        if (pPlayer == gMe && gGameOptions.nGameType == GAMETYPE_0)
            sfxSetReverb(1);
        break;
    case 119:
        pPlayer->ata1[4]++;
        break;
    case 139:
        pPlayer->ata1[1]++;
        break;
    case 117: // guns akimbo
        if (!VanillaMode() && !powerupAkimboWeapons(pPlayer->atbd)) // if weapon doesn't have a akimbo state, don't raise weapon
            break;
        pPlayer->atc.newWeapon = pPlayer->atbd;
        WeaponRaise(pPlayer);
        break;
    }
    sfxPlay3DSound(pPlayer->pSprite, 776, -1, 0);
    return 1;
}

void powerupDeactivate(PLAYER *pPlayer, int nPowerUp)
{
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0)
        pPlayer->packInfo[nPack].at0 = 0;
    switch (nPowerUp+100)
    {
    case 112:
    case 115: // jump boots
        pPlayer->ata1[0]--;
        break;
    case 114: // death mask
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]--;
        break;
    }
    case 118: // diving suit
        pPlayer->ata1[4]--;
        if ((pPlayer == gMe) && (VanillaMode() || !powerupCheck(pPlayer, 24)))
            sfxSetReverb(0);
        break;
    case 124: // reflective shots
        if ((pPlayer == gMe) && (VanillaMode() || !packItemActive(pPlayer, 1)))
            sfxSetReverb(0);
        break;
    case 119:
        pPlayer->ata1[4]--;
        break;
    case 139:
        pPlayer->ata1[1]--;
        break;
    case 117: // guns akimbo
        if (!VanillaMode() && !powerupAkimboWeapons(pPlayer->atbd)) // if weapon doesn't have a akimbo state, don't raise weapon
            break;
        pPlayer->atc.newWeapon = pPlayer->atbd;
        WeaponRaise(pPlayer);
        break;
    }
}

static void powerupSetState(PLAYER *pPlayer, int nPowerUp, BOOL bState)
{
    if (bState)
        powerupActivate(pPlayer, nPowerUp);
    else
        powerupDeactivate(pPlayer, nPowerUp);
}

void powerupProcess(PLAYER *pPlayer)
{
    pPlayer->at31d = ClipLow(pPlayer->at31d-4, 0);
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        int nPack = powerupToPackItem(i);
        if (nPack >= 0)
        {
            if (pPlayer->packInfo[nPack].at0)
            {
                pPlayer->at202[i] = ClipLow(pPlayer->at202[i]-4, 0);
                if (pPlayer->at202[i])
                    pPlayer->packInfo[nPack].at1 = (100*pPlayer->at202[i])/gPowerUpInfo[i].at3;
                else
                {
                    powerupDeactivate(pPlayer, i);
                    if (pPlayer->at321 == nPack)
                        pPlayer->at321 = 0;
                }
            }
        }
        else
        {
            if (pPlayer->at202[i] > 0)
            {
                pPlayer->at202[i] = ClipLow(pPlayer->at202[i]-4, 0);
                if (!pPlayer->at202[i])
                    powerupDeactivate(pPlayer, i);
            }
        }
    }
}

void powerupClear(PLAYER *pPlayer)
{
    if (!VanillaMode() && (pPlayer == gMe)) // turn off reverb sound effects
    {
        if (packItemActive(pPlayer, 1) || powerupCheck(pPlayer, 24)) // if diving suit/reflective shots powerup is active, turn off reverb effect
            sfxSetReverb(0);
    }
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        pPlayer->at202[i] = 0;
    }
}

void powerupInit(void)
{
}

static int packItemToPowerup(int nPack)
{
    int nPowerUp = -1;
    switch (nPack)
    {
    case 0:
        break;
    case 1:
        nPowerUp = 18;
        break;
    case 2:
        nPowerUp = 21;
        break;
    case 3:
        nPowerUp = 25;
        break;
    case 4:
        nPowerUp = 15;
        break;
    default:
        ThrowError(745)("Unhandled pack item %d", nPack);
        break;
    }
    return nPowerUp;
}

static int powerupToPackItem(int nPowerUp)
{
    switch (nPowerUp)
    {
    case 18:
        return 1;
    case 21:
        return 2;
    case 25:
        return 3;
    case 15:
        return 4;
    }
    return -1;
}

BOOL packAddItem(PLAYER *pPlayer, int nPack)
{
    switch (nPack)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        {
            if (pPlayer->packInfo[nPack].at1 >= 100)
                return 0;
            pPlayer->packInfo[nPack].at1 = 100;
            int nPowerUp = packItemToPowerup(nPack);
            if (nPowerUp >= 0)
                pPlayer->at202[nPowerUp] = gPowerUpInfo[nPowerUp].at3;
            if (pPlayer->at321 == -1)
                pPlayer->at321 = nPack;
            if (!pPlayer->packInfo[pPlayer->at321].at1)
                pPlayer->at321 = nPack;
            break;
        }
        default:
            ThrowError(830)("Unhandled pack item %d", nPack);
            break;
    }
    return 1;
}

int packCheckItem(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packInfo[nPack].at1;
}

BOOL packItemActive(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packInfo[nPack].at0;
}

void packUseItem(PLAYER *pPlayer, int nPack)
{
    PACKINFO *pPackInfo = &pPlayer->packInfo[nPack];
    BOOL v4 = 0;
    int nPowerUp = -1;
    if (pPackInfo->at1 > 0)
    {
        switch (nPack)
        {
        case 0:
        {
            XSPRITE *pXSprite = pPlayer->pXSprite;
            if ((pXSprite->health >> 4) < 100)
            {
                int heal = ClipHigh(100-(pXSprite->health >> 4), pPlayer->packInfo[nPack].at1);
                actHealDude(pXSprite, heal, 100);
                pPlayer->packInfo[nPack].at1 -= heal;
            }
            break;
        }
        case 1:
            v4 = 1;
            nPowerUp = 18;
            break;
        case 2:
            v4 = 1;
            nPowerUp = 21;
            break;
        case 3:
            v4 = 1;
            nPowerUp = 25;
            break;
        case 4:
            v4 = 1;
            nPowerUp = 15;
            break;
        default:
            ThrowError(925)("Unhandled pack item %d", nPack);
            return;
        }
    }
    pPlayer->at31d = 0;
    if (v4)
        powerupSetState(pPlayer, nPowerUp, !pPlayer->packInfo[nPack].at0);
}

void packPrevItem(PLAYER *pPlayer)
{
    if (pPlayer->at31d > 0)
    {
        for (int nPrev = ClipLow(pPlayer->at321-1,0); nPrev >= 0; nPrev--)
        {
            if (pPlayer->packInfo[nPrev].at1)
            {
                pPlayer->at321 = nPrev;
                break;
            }
        }
    }
    pPlayer->at31d = 600;
}

void packNextItem(PLAYER *pPlayer)
{
    if (pPlayer->at31d > 0)
    {
        for (int nNext = ClipHigh(pPlayer->at321+1,5); nNext < 5; nNext++)
        {
            if (pPlayer->packInfo[nNext].at1)
            {
                pPlayer->at321 = nNext;
                break;
            }
        }
    }
    pPlayer->at31d = 600;
}


BOOL playerSeqPlaying(PLAYER *pPlayer, int nSeq)
{
    int nCurSeq = seqGetID(3, pPlayer->pSprite->extra);
    if (nCurSeq == pPlayer->pDudeInfo->seqStartID+nSeq && seqGetStatus(3,pPlayer->pSprite->extra) >= 0)
        return 1;
    return 0;
}

void playerSetRace(PLAYER *pPlayer, int nLifeMode)
{
    dassert(nLifeMode >= kModeHuman && nLifeMode <= kModeBeast, 1020);
    pPlayer->at5f = nLifeMode;
    DUDEINFO *pDudeInfo = pPlayer->pDudeInfo;
    *pDudeInfo = gPlayerTemplate[nLifeMode];
    for (int i = 0; i < 7; i++)
    {
        int t = Handicap[gProfile[pPlayer->at57].skill];
        pDudeInfo->at70[i] = mulscale8(t, pDudeInfo->at54[i]);
    }
}

void playerSetGodMode(PLAYER *pPlayer, BOOL bGodMode)
{
    if (bGodMode)
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]++;
    }
    else
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]--;
    }
    pPlayer->at31a = bGodMode;
}

void playerResetInertia(PLAYER *pPlayer)
{
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
    pPlayer->at67 = pPlayer->pSprite->z-pPosture->at24;
    pPlayer->at6f = pPlayer->pSprite->z-pPosture->at28;
    viewBackupView(pPlayer->at57);
}

void playerCorrectInertia(PLAYER *pPlayer, VECTOR3D *pOldpos)
{
    pPlayer->at67 += pPlayer->pSprite->z-pOldpos->dz;
    pPlayer->at6f += pPlayer->pSprite->z-pOldpos->dz;
    viewCorrectViewOffsets(pPlayer->at57, pOldpos);
}

void playerResetPosture(PLAYER* pPlayer) {
    pPlayer->at37 = 0;
    pPlayer->at3b = 0;
    pPlayer->at4b = 0;
    pPlayer->at3f = 0;
    pPlayer->at43 = 0;
    pPlayer->at4f = 0;
    pPlayer->at53 = 0;
}

void playerStart(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    INPUT *pInput = &pPlayer->atc;
    int i;
    ZONE *pStartZone;
    if (gGameOptions.nGameType >= GAMETYPE_2)
        pStartZone = &gStartZone[Random(8)];
    else
        pStartZone = &gStartZone[nPlayer];
    if (!VanillaMode())
        sfxKillAllSounds(pPlayer->pSprite);
    SPRITE *pSprite = actSpawnSprite(pStartZone->sectnum, pStartZone->x, pStartZone->y, pStartZone->z, 6, 1);
    dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites, 1094);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    pPlayer->pSprite = pSprite;
    pPlayer->at5b = pSprite->index;
    pPlayer->pXSprite = pXSprite;
    DUDEINFO *pDudeInfo = &dudeInfo[nPlayer + kDudePlayer1 - kDudeBase];
    pPlayer->pDudeInfo = pDudeInfo;
    playerSetRace(pPlayer, kModeHuman);
    if (!VanillaMode())
        playerResetPosture(pPlayer);
    seqSpawn(pDudeInfo->seqStartID, 3, pSprite->extra);
    if (pPlayer == gMe)
        SetBitString(show2dsprite, pSprite->index);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z -= bottom - pSprite->z;
    pSprite->pal = 11+(pPlayer->at2ea&3);
    pSprite->ang = pStartZone->ang;
    pSprite->type = kDudePlayer1+nPlayer;
    pSprite->clipdist = pDudeInfo->ata;
    pSprite->flags = 15;
    pXSprite->at2c_0 = 0;
    pXSprite->at2e_0 = -1;
    pPlayer->pXSprite->health = pDudeInfo->at2<<4;
    pPlayer->pSprite->cstat &= ~32768;
    pPlayer->at63 = 0;
    pPlayer->at7b = 0;
    pPlayer->at7f = 0;
    pPlayer->at77 = 0;
    pPlayer->at83 = 0;
    pPlayer->at2ee = -1;
    pPlayer->at2f2 = 1200;
    pPlayer->at2f6 = 0;
    pPlayer->at2fa = 0;
    pPlayer->at2fe = 0;
    pPlayer->at302 = 0;
    pPlayer->at306 = 0;
    pPlayer->at30a = 0;
    pPlayer->at30e = 0;
    pPlayer->at312 = 0;
    pPlayer->at316 = 0;
    pPlayer->at2f = 0;
    pPlayer->playerVoodooTarget = -1;
    pPlayer->at34e = 0;
    pPlayer->at352 = 0;
    pPlayer->at356 = 0;
    playerResetInertia(pPlayer);
    pPlayer->at6b = pPlayer->at73 = 0;
    pPlayer->at1ca.dx = 0x4000;
    pPlayer->at1ca.dy = 0;
    pPlayer->at1ca.dz = 0;
    pPlayer->at1d6 = -1;
    for (i = 0; i < 8; i++)
        pPlayer->at88[i] = gGameOptions.nGameType >= GAMETYPE_2;
    pPlayer->at90 = 0;
    for (i = 0; i < 8; i++)
        pPlayer->at91[i] = -1;
    for (i = 0; i < 7; i++)
        pPlayer->ata1[i] = 0;
    if (pPlayer->at31a)
        playerSetGodMode(pPlayer, 1);
    gInfiniteAmmo = 0;
    gFullMap = 0;
    pPlayer->at1ba = 0;
    pPlayer->at1fe = 0;
    pPlayer->atbe = 0;
    xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
    pInput->forward = 0;
    pInput->strafe = 0;
    pInput->turn = 0;
    pInput->mlook = 0;
    pInput->buttonFlags.byte = 0;
    pInput->keyFlags.word = 0;
    pInput->useFlags.byte = 0;
    pPlayer->at35a = 0;
    pPlayer->at37f = 0;
    pPlayer->at35e = 0;
    pPlayer->at362 = 0;
    pPlayer->at366 = 0;
    pPlayer->at36a = 0;
    pPlayer->at36e = 0;
    pPlayer->at372 = 0;
    pPlayer->atbf = 0;
    pPlayer->atc3 = 0;
    pPlayer->at26 = -1;
    pPlayer->at376 = 0;
    for (i = 0; i < kMaxPowerUps; i++)
    {
        if (!VanillaMode() && (i == 15 || i == 18 || i == 21 || i == 25)) // don't reset jump boots/diving suit/crystal ball/beast vision between levels
            continue;
        pPlayer->at202[i] = 0;
    }
    if (pPlayer == gMe)
    {
        viewInitializePrediction();
        gViewMap.SetPos(pPlayer->pSprite->x, pPlayer->pSprite->y);
        gViewMap.SetAngle(pPlayer->pSprite->ang);
        if (!VanillaMode())
            sfxResetListener(); // player is listener, update ear position/reset ear velocity so audio pitch of surrounding sfx does not freak out when respawning player
    }
    if (IsUnderwaterSector(pSprite->sectnum))
    {
        pPlayer->at2f = 1;
        pPlayer->pXSprite->at17_6 = 1;
    }
}

int defaultOrder1[] = {
    3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
};
int defaultOrder2[] = {
    3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
};

void playerReset(PLAYER *pPlayer)
{
    dassert(pPlayer != NULL, 1271);
    for (int i = 0; i < 14; i++)
    {
        pPlayer->atcb[i] = gInfiniteAmmo;
        pPlayer->atd9[i] = 0;
    }
    pPlayer->atcb[1] = 1;
    pPlayer->atbd = 0;
    pPlayer->at2a = -1;
    pPlayer->atc.newWeapon = 1;
    for (i = 0; i < 14; i++)
    {
        pPlayer->at111[0][i] = defaultOrder1[i];
        pPlayer->at111[1][i] = defaultOrder2[i];
    }
    for (i = 0; i < 12; i++)
    {
        pPlayer->at181[i] = gInfiniteAmmo ? gAmmoInfo[i].at0 : 0;
    }
    for (i = 0; i < 3; i++)
        pPlayer->at33e[i] = 0;
    pPlayer->atbf = 0;
    pPlayer->atc3 = 0;
    pPlayer->at26 = -1;
    pPlayer->at1b1 = 0;
    pPlayer->at321 = -1;
    for (i = 0; i < 5; i++)
    {
        pPlayer->packInfo[i].at0 = 0;
        pPlayer->packInfo[i].at1 = 0;
    }
}

void playerInit(int nPlayer, unsigned int a2)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    if (!(a2&1))
        memset(pPlayer, 0, sizeof(PLAYER));
    pPlayer->at57 = nPlayer;
    pPlayer->at2ea = nPlayer;
    if (gGameOptions.nGameType == GAMETYPE_3)
        pPlayer->at2ea &= 1;
    pPlayer->at2c6 = 0;
    memset(int_21EFB0, 0, sizeof(int_21EFB0));
    memset(int_21EFD0, 0, sizeof(int_21EFD0));
    memset(pPlayer->at2ca, 0, sizeof(pPlayer->at2ca));
    if (!(a2&1))
        playerReset(pPlayer);
}

BOOL func_3A158(PLAYER *a1, SPRITE *a2)
{
    for (int nSprite = headspritestat[4]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        if (a2 && a2->index == nSprite)
            continue;
        SPRITE *pSprite = &sprite[nSprite];
        if (pSprite->type == 431 && actOwnerIdToSpriteId(pSprite->owner) == a1->at5b)
            return 1;
    }
    return 0;
}

static BOOL PickupItem(PLAYER *pPlayer, SPRITE *pItem)
{
    char buffer[80];
    SPRITE *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int pickupSnd = 775;
    int nType = pItem->type - 100;
    switch (pItem->type)
    {
    case 145:
    case 146:
        if (gGameOptions.nGameType != GAMETYPE_3)
            return 0;
        if (pItem->extra > 0)
        {
            XSPRITE *pXItem = &xsprite[pItem->extra];
            if (pItem->type == 145)
            {
                if (pPlayer->at2ea == 1)
                {
                    if ((pPlayer->at90&1) == 0 && pXItem->at1_6)
                    {
                        pPlayer->at90 |= 1;
                        pPlayer->at91[0] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, 0);
                        sprintf(buffer, "%s stole Blue Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8007, 255, 2);
                        viewSetMessage(buffer);
                    }
                }
                if (pPlayer->at2ea == 0)
                {
                    if ((pPlayer->at90&1) != 0 && !pXItem->at1_6)
                    {
                        pPlayer->at90 &= ~1;
                        pPlayer->at91[0] = -1;
                        trTriggerSprite(pItem->index, pXItem, 1);
                        sprintf(buffer, "%s returned Blue Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8003, 255, 2);
                        viewSetMessage(buffer);
                    }
                    if ((pPlayer->at90&2) != 0 && pXItem->at1_6)
                    {
                        pPlayer->at90 &= ~2;
                        pPlayer->at91[1] = -1;
                        int_21EFB0[pPlayer->at2ea] += 10;
                        int_21EFD0[pPlayer->at2ea] += 240;
                        evSend(0, 0, 81, COMMAND_ID_1);
                        sprintf(buffer, "%s captured Red Flag!", gProfile[pPlayer->at57].name);
                        sndStartSample(8001, 255, 2);
                        viewSetMessage(buffer);
                        if (int_28E3D4 == 3 && myconnectindex == connecthead)
                        {
                            sprintf(buffer, "frag A killed B\n");
                            func_7AC28(buffer);
                        }
                    }
                }
            }
            else if (pItem->type == 146)
            {
                if (pPlayer->at2ea == 0)
                {
                    if((pPlayer->at90&2) == 0 && pXItem->at1_6)
                    {
                        pPlayer->at90 |= 2;
                        pPlayer->at91[1] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, 0);
                        sprintf(buffer, "%s stole Red Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8006, 255, 2);
                        viewSetMessage(buffer);
                    }
                }
                if (pPlayer->at2ea == 1)
                {
                    if ((pPlayer->at90&2) != 0 && !pXItem->at1_6)
                    {
                        pPlayer->at90 &= ~2;
                        pPlayer->at91[1] = -1;
                        trTriggerSprite(pItem->index, pXItem, 1);
                        sprintf(buffer, "%s returned Red Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8002, 255, 2);
                        viewSetMessage(buffer);
                    }
                    if ((pPlayer->at90&1) != 0 && pXItem->at1_6)
                    {
                        pPlayer->at90 &= ~1;
                        pPlayer->at91[0] = -1;
                        int_21EFB0[pPlayer->at2ea] += 10;
                        int_21EFD0[pPlayer->at2ea] += 240;
                        evSend(0, 0, 80, COMMAND_ID_1);
                        sprintf(buffer, "%s captured Blue Flag!", gProfile[pPlayer->at57].name);
                        sndStartSample(8000, 255, 2);
                        viewSetMessage(buffer);
                        if (int_28E3D4 == 3 && myconnectindex == connecthead)
                        {
                            sprintf(buffer, "frag B killed A\n");
                            func_7AC28(buffer);
                        }
                    }
                }
            }
        }
        return 0;
    case 147:
        if (gGameOptions.nGameType != GAMETYPE_3)
            return 0;
        evKill(pItem->index, 3, CALLBACK_ID_17);
        pPlayer->at91[0] = pItem->owner;
        pPlayer->at90 |= 1;
        break;
    case 148:
        if (gGameOptions.nGameType != GAMETYPE_3)
            return 0;
        evKill(pItem->index, 3, CALLBACK_ID_17);
        pPlayer->at91[1] = pItem->owner;
        pPlayer->at90 |= 2;
        break;
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
    {
        ARMORDATA *pArmorData = &armorData[pItem->type-140];
        BOOL va = 0;
        if (pPlayer->at33e[1] < pArmorData->atc)
        {
            pPlayer->at33e[1] = ClipHigh(pPlayer->at33e[1]+pArmorData->at8, pArmorData->atc);
            va = 1;
        }
        if (pPlayer->at33e[0] < pArmorData->at4)
        {
            pPlayer->at33e[0] = ClipHigh(pPlayer->at33e[0]+pArmorData->at0, pArmorData->at4);
            va = 1;
        }
        if (pPlayer->at33e[2] < pArmorData->at14)
        {
            pPlayer->at33e[2] = ClipHigh(pPlayer->at33e[2]+pArmorData->at10, pArmorData->at14);
            va = 1;
        }
        if (!va)
            return 0;
        pickupSnd = 779;
        break;
    }
    case 121:
        if (gGameOptions.nGameType == GAMETYPE_0 || !packAddItem(pPlayer, gItemData[nType].at8))
            return 0;
        break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
        if (pPlayer->at88[pItem->type-99])
            return 0;
        pPlayer->at88[pItem->type-99] = 1;
        pickupSnd = 781;
        break;
    case 111:
    case 108:
    case 109:
    case 110:
        if (!actHealDude(pXSprite, gPowerUpInfo[nType].at3, gPowerUpInfo[nType].at7))
            return 0;
        return 1;
    case 107:
    case 115:
    case 118:
    case 125:
        if (!packAddItem(pPlayer, gItemData[nType].at8))
            return 0;
        break;
    default:
        if (!powerupActivate(pPlayer, nType))
            return 0;
        return 1;
    }
    sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, pickupSnd, pSprite->sectnum);
    return 1;
}

static BOOL PickupAmmo(PLAYER *pPlayer, SPRITE *pAmmo)
{
    AMMOITEMDATA *pAmmoItemData = &gAmmoItemData[pAmmo->type-60];
    int nAmmoType = pAmmoItemData->ata;
    if (pPlayer->at181[nAmmoType] >= gAmmoInfo[nAmmoType].at0)
        return 0;
    pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType]+pAmmoItemData->at8, gAmmoInfo[nAmmoType].at0);
    if (pAmmoItemData->atb != 0)
        pPlayer->atcb[pAmmoItemData->atb] = 1;
    SPRITE *pSprite = pPlayer->pSprite;
    sfxPlay3DSound(pSprite, 782);
    return 1;
}

static BOOL PickupWeapon(PLAYER *pPlayer, SPRITE *pWeapon)
{
    WEAPONITEMDATA *pWeaponItemData = &gWeaponItemData[pWeapon->type-40];
    int nWeaponType = pWeaponItemData->at8;
    int nAmmoType = pWeaponItemData->ata;
    if (!pPlayer->atcb[nWeaponType] || gGameOptions.nWeaponSettings == WEAPONSETTINGS_2 || gGameOptions.nWeaponSettings == WEAPONSETTINGS_3)
    {
        if (pWeapon->type == 50 && gGameOptions.nGameType > GAMETYPE_1 && func_3A158(pPlayer, NULL))
            return 0;
        pPlayer->atcb[nWeaponType] = 1;
        if (nAmmoType != -1)
            pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType]+pWeaponItemData->atc, gAmmoInfo[nAmmoType].at0);
        SPRITE *pSprite = pPlayer->pSprite;
        byte nNewWeapon = WeaponUpgrade(pPlayer, nWeaponType);
        if (nNewWeapon != pPlayer->atbd)
        {
            pPlayer->atc3 = 0;
            pPlayer->atbe = nNewWeapon;
        }
        sfxPlay3DSound(pSprite, 777);
        return 1;
    }
    if (actGetRespawnTime(pWeapon) == 0)
        return 0;
    if (nAmmoType == -1)
        return 0;
    if (pPlayer->at181[nAmmoType] >= gAmmoInfo[nAmmoType].at0)
        return 0;
    pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType]+pWeaponItemData->atc, gAmmoInfo[nAmmoType].at0);
    SPRITE *pSprite = pPlayer->pSprite;
    sfxPlay3DSound(pPlayer->pSprite, 777);
    return 1;
}

static void PickUp(PLAYER *pPlayer, SPRITE *pSprite)
{
    char buffer[80];
    BOOL vb = 0;
    int nType = pSprite->type;
    if (nType >= 100 && nType <= 149)
    {
        vb = PickupItem(pPlayer, pSprite);
        sprintf(buffer, "Picked up %s", gItemText[nType-100]);
    }
    else if (nType >= 60 && nType < 81)
    {
        vb = PickupAmmo(pPlayer, pSprite);
        sprintf(buffer, "Picked up %s", gAmmoText[nType-60]);
    }
    else if (nType >= 40 && nType < 51)
    {
        vb = PickupWeapon(pPlayer, pSprite);
        sprintf(buffer, "Picked up %s", gWeaponText[nType-40]);
    }
    if (vb)
    {
        if (pSprite->extra > 0)
        {
            XSPRITE *pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->ate_1)
                trTriggerSprite(pSprite->index, pXSprite, 32);
        }
        if (!actCheckRespawn(pSprite))
            actPostSprite(pSprite->index, kStatFree);
        pPlayer->at377 = 30;
        if (pPlayer == gMe)
            viewSetMessage(buffer);
    }
}

static void CheckPickUp(PLAYER *pPlayer)
{
    SPRITE *pSprite = pPlayer->pSprite;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector = pSprite->sectnum;
    int nSprite;
    int nNextSprite;
    int dx, dy, dz;
    for (nSprite = headspritestat[3]; nSprite >= 0; nSprite = nNextSprite)
    {
        nNextSprite = nextspritestat[nSprite];
        SPRITE *pItem = &sprite[nSprite];
        if (pItem->flags&kSpriteFlag5)
            continue;
        dx = klabs(x-pItem->x)>>4;
        if (dx > 48)
            continue;
        dy = klabs(y-pItem->y)>>4;
        if (dy > 48)
            continue;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        dz = 0;
        if (pItem->z < top)
            dz = (top-pItem->z)>>8;
        else if (pItem->z > bottom)
            dz = (pItem->z-bottom)>>8;
        if (dz > 32)
            continue;
        if (approxDist(dx,dy) > 48)
            continue;
        GetSpriteExtents(pItem, &top, &bottom);
        if (cansee(x, y, z, nSector, pItem->x, pItem->y, pItem->z, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, top, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, bottom, pItem->sectnum))
            PickUp(pPlayer, pItem);
    }
}

static int ActionScan(PLAYER *pPlayer, int *a2, int *a3)
{
    SPRITE *pSprite = pPlayer->pSprite;
    *a2 = 0;
    *a3 = 0;
    int x = Cos(pSprite->ang)>>16;
    int y = Sin(pSprite->ang)>>16;
    int z = pPlayer->at83;
    int hit = HitScan(pSprite, pPlayer->at67, x, y, z, 0x10000040, 128);
    int hitDist = approxDist(pSprite->x-gHitInfo.hitx, pSprite->y-gHitInfo.hity) >> 4;
    if (hitDist < 64)
    {
        switch (hit)
        {
        case 3:
            *a2 = gHitInfo.hitsprite;
            *a3 = sprite[*a2].extra;
            if (*a3 > 0 && sprite[*a2].statnum == 4)
            {
                SPRITE *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                if (pSprite->type == 431)
                {
                    if (gGameOptions.nGameType > GAMETYPE_1 && func_3A158(pPlayer, pSprite))
                        return -1;
                    pXSprite->at18_2 = pPlayer->at57;
                    pXSprite->atd_2 = 0;
                }
            }
            if (*a3 > 0 && xsprite[*a3].atd_6)
                return 3;
            if (sprite[*a2].statnum == 6)
            {
                SPRITE *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                int nMass = dudeInfo[pSprite->type-kDudeBase].at4;
                if (nMass)
                {
                    int t2 = divscale8(0xccccc, nMass);
                    xvel[*a2] += mulscale16(x, t2);
                    yvel[*a2] += mulscale16(y, t2);
                    zvel[*a2] += mulscale16(z, t2);
                }
                if (pXSprite->atd_6 && !pXSprite->at1_6 && !pXSprite->atd_2)
                    trTriggerSprite(*a2, pXSprite, 30);
            }
            break;
        case 0:
        case 4:
            *a2 = gHitInfo.hitwall;
            *a3 = wall[*a2].extra;
            if (*a3 > 0 && xwall[*a3].at10_5)
                return 0;
            if (wall[*a2].nextsector >= 0)
            {
                *a2 = wall[*a2].nextsector;
                *a3 = sector[*a2].extra;
                if (*a3 > 0 && xsector[*a3].at17_7)
                    return 6;
            }
            break;
        case 1:
        case 2:
            *a2 = gHitInfo.hitsect;
            *a3 = sector[*a2].extra;
            if (*a3 > 0 && xsector[*a3].at17_2)
                return 6;
            break;
        }
    }
    *a2 = pSprite->sectnum;
    *a3 = sector[*a2].extra;
    if (*a3 > 0 && xsector[*a3].at17_2)
        return 6;
    return -1;
}

static void ProcessInput(PLAYER *pPlayer)
{
    SPRITE *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nSprite = pPlayer->at5b;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
    INPUT *pInput = &pPlayer->atc;
    pPlayer->at2e = VanillaMode() ? pInput->syncFlags.run : 0;
    if (pInput->buttonFlags.byte || pInput->forward || pInput->strafe || pInput->turn)
        pPlayer->at30a = 0;
    else if (pPlayer->at30a >= 0)
        pPlayer->at30a += 4;
    if (VanillaMode() || (pXSprite->health == 0))
        WeaponProcess(pPlayer);
    if (pXSprite->health == 0)
    {
        BOOL bSeqStat = playerSeqPlaying(pPlayer, 16);
        if (pPlayer->at2ee != -1)
        {
            int dx = sprite[pPlayer->at2ee].x-pSprite->x;
            int dy = sprite[pPlayer->at2ee].y-pSprite->y;
            pSprite->ang = getangle(dx, dy);
        }
        pPlayer->at1fe += 4;
        if (!bSeqStat)
            pPlayer->at7b = mulscale16(0x8000-(Cos(ClipHigh(pPlayer->at1fe*8, 1024))>>15), 120);
        if (pPlayer->atbd)
            pInput->newWeapon = pPlayer->atbd;
        if (pInput->keyFlags.action)
        {
            if (bSeqStat)
            {
                if (pPlayer->at1fe > 360)
                    seqSpawn(pPlayer->pDudeInfo->seqStartID+14, 3, pPlayer->pSprite->extra, nPlayerSurviveClient);
            }
            else if (seqGetStatus(3, pPlayer->pSprite->extra) < 0)
            {
                if (pPlayer->pSprite)
                    pPlayer->pSprite->type = 426;
                actPostSprite(pPlayer->at5b, 4);
                seqSpawn(pPlayer->pDudeInfo->seqStartID+15, 3, pPlayer->pSprite->extra, -1);
                playerReset(pPlayer);
                if (gGameOptions.nGameType == GAMETYPE_0 && numplayers == 1)
                {
                    if (gDemo.at0)
                        gDemo.Close();
                    pInput->keyFlags.restart = 1;
                }
                else
                    playerStart(pPlayer->at57);
            }
            pInput->keyFlags.action = 0;
        }
        return;
    }
    if (pPlayer->at2f == 1 || gFlyMode)
    {
        int vel = 0;
        int sinVal = Sin(pSprite->ang);
        int cosVal = Cos(pSprite->ang);
        if (pInput->forward)
        {
            if (pInput->forward > 0)
                vel = pInput->forward * pPosture->at0;
            else
                vel = pInput->forward * pPosture->at8;
            xvel[nSprite] += mulscale30(vel, cosVal);
            yvel[nSprite] += mulscale30(vel, sinVal);
        }
        if (pInput->strafe)
        {
            vel = pInput->strafe * pPosture->at4;
            xvel[nSprite] += mulscale30(vel, sinVal);
            yvel[nSprite] -= mulscale30(vel, cosVal);
        }
    }
    else if (pXSprite->at30_0 < 256)
    {
        int drag = 0x10000;
        int vel = 0;
        if (pXSprite->at30_0 > 0)
            drag -= divscale16(pXSprite->at30_0, 256);
        int sinVal = Sin(pSprite->ang);
        int cosVal = Cos(pSprite->ang);
        if (pInput->forward)
        {
            if (pInput->forward > 0)
                vel = pInput->forward * pPosture->at0;
            else
                vel = pInput->forward * pPosture->at8;
            if (pXSprite->at30_0)
                vel = mulscale16(vel, drag);
            xvel[nSprite] += mulscale30(vel, cosVal);
            yvel[nSprite] += mulscale30(vel, sinVal);
        }
        if (pInput->strafe)
        {
            vel = pInput->strafe * pPosture->at4;
            if (pXSprite->at30_0)
                vel = mulscale16(vel, drag);
            xvel[nSprite] += mulscale30(vel, sinVal);
            yvel[nSprite] -= mulscale30(vel, cosVal);
        }
    }
    if (pInput->turn != 0)
        pSprite->ang = (pSprite->ang+((pInput->turn<<2)>>4))&2047;
    
    if (pInput->keyFlags.spin180)
    {
        if (!pPlayer->at316)
            pPlayer->at316 = -1024;
        pInput->keyFlags.spin180 = 0;
    }
    if (pPlayer->at316 < 0)
    {
        int speed = pPlayer->at2f == 1 ? 64 : 128;
        pPlayer->at316 = ClipHigh(pPlayer->at316+speed, 0);
        pSprite->ang += (short)speed;
    }
    if (!pInput->buttonFlags.jump)
        pPlayer->at31c = 0;
    switch (pPlayer->at2f)
    {
        case 1:
            if (pInput->buttonFlags.jump)
                zvel[nSprite] -= 23301;
            if (pInput->buttonFlags.crouch)
                zvel[nSprite] += 23301;
            break;
        case 2:
            if (!pInput->buttonFlags.crouch)
                pPlayer->at2f = 0;
            break;
        default:
            if (gFlyMode)
                break;
            if (!pPlayer->at31c && pInput->buttonFlags.jump && pXSprite->at30_0 == 0)
            {
                sfxPlay3DSound(pSprite, 700, 0);
                if (packItemActive(pPlayer, 4))
                    zvel[nSprite] = -1529173;
                else
                    zvel[nSprite] = -764586;
                pPlayer->at31c = 1;
            }
            if (pInput->buttonFlags.crouch)
                pPlayer->at2f = 2;
            break;
    }
    if (pInput->keyFlags.action)
    {
        int a2, a3;
        int key;
        switch (ActionScan(pPlayer, &a2, &a3))
        {
        case 6:
            if (a3 > 0 && a3 <= 2048)
            {
                XSECTOR *pXSector = &xsector[a3];
                key = pXSector->at16_7;
                if (pXSector->at35_0 && pPlayer == gMe)
                {
                    viewSetMessage("It's locked");
                    sndStartSample(3062, 255, 2);
                }
                if (!key || pPlayer->at88[key])
                    trTriggerSector(a2, pXSector, 30);
                else if (pPlayer == gMe)
                {
                    viewSetMessage("That requires a key.");
                    sndStartSample(3063, 255, 2);
                }
            }
            break;
        case 0:
        {
            XWALL *pXWall = &xwall[a3];
            key = pXWall->at10_2;
            if (pXWall->at13_2 && pPlayer == gMe)
            {
                viewSetMessage("It's locked");
                sndStartSample(3062, 255, 2);
            }
            if (!key || pPlayer->at88[key])
                trTriggerWall(a2, pXWall, 50);
            else if (pPlayer == gMe)
            {
                viewSetMessage("That requires a key.");
                sndStartSample(3063, 255, 2);
            }
            break;
        }
        case 3:
        {
            XSPRITE *pXSprite = &xsprite[a3];
            key = pXSprite->atd_3;
            if (pXSprite->at17_5 && pPlayer == gMe)
            {
                if (pXSprite->at1b_0)
                    trTextOver(pXSprite->at1b_0);
            }
            if (!key || pPlayer->at88[key])
                trTriggerSprite(a2, pXSprite, 30);
            else if (pPlayer == gMe)
            {
                viewSetMessage("That requires a key.");
                sndStartSample(3063, 255, 2);
            }
            break;
        }
        }
        if (pPlayer->at372 > 0)
            pPlayer->at372 = ClipLow(pPlayer->at372-4*(6-gGameOptions.nDifficulty), 0);
        if (pPlayer->at372 <= 0 && pPlayer->at376)
        {
            SPRITE *pSprite2 = actSpawnDude(pPlayer->pSprite, 212, pPlayer->pSprite->clipdist<<1, 0);
            pSprite2->ang = (pPlayer->pSprite->ang+1024)&2047;
            int nSprite = pPlayer->pSprite->index;
            int x = Cos(pPlayer->pSprite->ang)>>16;
            int y = Sin(pPlayer->pSprite->ang)>>16;
            xvel[pSprite2->index] = xvel[nSprite] + mulscale14(0x155555, x);
            yvel[pSprite2->index] = yvel[nSprite] + mulscale14(0x155555, y);
            zvel[pSprite2->index] = zvel[nSprite];
            pPlayer->at376 = 0;
        }
        pInput->keyFlags.action = 0;
    }
    if (pInput->keyFlags.lookCenter && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
    {
        if (pPlayer->at77 < 0)
        {
            pPlayer->at77 = ClipHigh(pPlayer->at77 + 4, 0);
        }
        if (pPlayer->at77 > 0)
        {
            pPlayer->at77 = ClipLow(pPlayer->at77 - 4, 0);
        }
        if (pPlayer->at77 == 0)
            pInput->keyFlags.lookCenter = 0;
    }
    else
    {
        if (pInput->buttonFlags.lookUp)
        {
            pPlayer->at77 = ClipHigh(pPlayer->at77 + 4, 60);
        }
        if (pInput->buttonFlags.lookDown)
        {
            pPlayer->at77 = ClipLow(pPlayer->at77 - 4, -60);
        }
    }
    if (pInput->mlook < 0)
    {
        pPlayer->at77 = (schar)ClipRange(pPlayer->at77 + ((pInput->mlook+3)>>2), -60, 60);
    }
    else
    {
        pPlayer->at77 = (schar)ClipRange(pPlayer->at77 + (pInput->mlook>>2), -60, 60);
    }

    if (pPlayer->at77 > 0)
    {
        pPlayer->at7b = mulscale30(120, Sin(pPlayer->at77 * 8));
    }
    else if (pPlayer->at77 < 0)
    {
        pPlayer->at7b = mulscale30(180, Sin(pPlayer->at77 * 8));
    }
    else
        pPlayer->at7b = 0;

    int nSector = pSprite->sectnum;
    int hit = gSpriteHit[pSprite->extra].florhit & 0xe000;
    BOOL onFloor = pXSprite->at30_0 < 16 && (hit == 0x4000 || hit == 0);
    if (onFloor && (sector[nSector].floorstat&kSectorStat1) != 0)
    {
        int floorZ = getflorzofslope(nSector, pSprite->x, pSprite->y);
        int newX = pSprite->x + mulscale30(64, Cos(pSprite->ang));
        int newY = pSprite->y + mulscale30(64, Sin(pSprite->ang));
        short newSector = nSector;
        updatesector(newX, newY, &newSector);
        if (newSector == nSector)
        {
            int newFloorZ = getflorzofslope(newSector, newX, newY);
            pPlayer->at7f = interpolate16(pPlayer->at7f, (floorZ-newFloorZ)>>3, 0x4000);
        }
    }
    else
    {
        pPlayer->at7f = interpolate16(pPlayer->at7f, 0, 0x4000);
        if (klabs(pPlayer->at7f) < 4)
            pPlayer->at7f = 0;
    }
    pPlayer->at83 = (-pPlayer->at7b)<<7;
    if (!VanillaMode())
        WeaponProcess(pPlayer);
    if (pInput->keyFlags.prevItem)
    {
        pInput->keyFlags.prevItem = 0;
        packPrevItem(pPlayer);
    }
    if (pInput->keyFlags.nextItem)
    {
        pInput->keyFlags.nextItem = 0;
        packNextItem(pPlayer);
    }
    if (pInput->keyFlags.useItem)
    {
        pInput->keyFlags.useItem = 0;
        if (pPlayer->packInfo[pPlayer->at321].at1 > 0)
            packUseItem(pPlayer, pPlayer->at321);
    }
    if (pInput->useFlags.useBeastVision)
    {
        pInput->useFlags.useBeastVision = 0;
        if (pPlayer->packInfo[3].at1 > 0)
            packUseItem(pPlayer, 3);
    }
    if (pInput->useFlags.useCrystalBall)
    {
        pInput->useFlags.useCrystalBall = 0;
        if (pPlayer->packInfo[2].at1 > 0)
            packUseItem(pPlayer, 2);
    }
    if (pInput->useFlags.useJumpBoots)
    {
        pInput->useFlags.useJumpBoots = 0;
        if (pPlayer->packInfo[4].at1 > 0)
            packUseItem(pPlayer, 4);
    }
    if (pInput->useFlags.useMedKit)
    {
        pInput->useFlags.useMedKit = 0;
        if (pPlayer->packInfo[0].at1 > 0)
            packUseItem(pPlayer, 0);
    }
    if (pInput->keyFlags.holsterWeapon)
    {
        pInput->keyFlags.holsterWeapon = 0;
        if (gGameOptions.uNetGameFlags&kNetGameFlagNoHolstering)
        {
            if (VanillaMode() || (pPlayer == gMe))
                viewSetMessage("Holstering is off in this match");
        }
        else if (pPlayer->atbd != 0)
        {
            WeaponLower(pPlayer);
            if (VanillaMode() || (pPlayer == gMe))
                viewSetMessage("Holstering weapon");
        }
    }
    CheckPickUp(pPlayer);
}

void playerProcess(PLAYER *pPlayer)
{
    SPRITE *pSprite = pPlayer->pSprite;
    int nSprite = pPlayer->at5b;
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
    powerupProcess(pPlayer);

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);

    int floordist = (bottom-pSprite->z)/4;
    int ceildist = (pSprite->z-top)/4;

    int clipdist = pSprite->clipdist<<2;

    if (!gNoClip)
    {
        short nSector = pSprite->sectnum;
        if (pushmove(&pSprite->x, &pSprite->y, &pSprite->z, &nSector, clipdist, ceildist, floordist, CLIPMASK0) == -1)
            actDamageSprite(nSprite, pSprite, kDamageFall, 500<<4);
        if (nSector != pSprite->sectnum)
        {
            if (nSector == -1)
            {
                nSector = pSprite->sectnum;
                actDamageSprite(nSprite, pSprite, kDamageFall, 500<<4);
            }
            dassert(nSector >= 0 && nSector < kMaxSectors, 2481);
            ChangeSpriteSect(nSprite, nSector);
        }
    }
    ProcessInput(pPlayer);

    int vel = approxDist(xvel[nSprite], yvel[nSprite]);
    
    int tmp;
    pPlayer->at6b = interpolate16(pPlayer->at6b, zvel[nSprite], 0x7000);
    tmp = pPlayer->pSprite->z - pPosture->at24 - pPlayer->at67;
    if (tmp > 0)
        pPlayer->at6b += mulscale16(tmp<<8, 40960);
    else
        pPlayer->at6b += mulscale16(tmp<<8, 6144);
    pPlayer->at67 += pPlayer->at6b >> 8;

    pPlayer->at73 = interpolate16(pPlayer->at73, zvel[nSprite], 0x5000);
    tmp = pPlayer->pSprite->z - pPosture->at28 - pPlayer->at6f;
    if (tmp > 0)
        pPlayer->at73 += mulscale16(tmp<<8, 32768);
    else
        pPlayer->at73 += mulscale16(tmp<<8, 3072);
    pPlayer->at6f += pPlayer->at73 >> 8;
    pPlayer->at37 = ClipLow(pPlayer->at37 - 4, 0);

    vel >>= 16;

    switch (pPlayer->at2f)
    {
        case 1:
            pPlayer->at3b = (pPlayer->at3b+17)&2047;
            pPlayer->at4b = (pPlayer->at4b+17)&2047;
            pPlayer->at3f = mulscale30(pPosture->at14*10,Sin(pPlayer->at3b*2));
            pPlayer->at43 = mulscale30(pPosture->at18*pPlayer->at37,Sin(pPlayer->at3b-256));
            pPlayer->at4f = mulscale30(pPosture->at1c*pPlayer->at37,Sin(pPlayer->at4b*2));
            pPlayer->at53 = mulscale30(pPosture->at20*pPlayer->at37,Sin(pPlayer->at4b-341));
            break;
        default:
            if (pXSprite->at30_0 < 256)
            {
                pPlayer->at3b = (pPlayer->at3b +(pPosture->atc[pPlayer->at2e]*4))&2047;
                pPlayer->at4b = (pPlayer->at4b+(pPosture->atc[pPlayer->at2e]*4)/2)&2047;
                if (pPlayer->at2e)
                {
                    if (pPlayer->at37 < 60) pPlayer->at37 = ClipHigh(pPlayer->at37 + vel, 60);
                }
                else
                {
                    if (pPlayer->at37 < 30) pPlayer->at37 = ClipHigh(pPlayer->at37 + vel, 30);
                }
            }
            pPlayer->at3f = mulscale30(pPosture->at14*pPlayer->at37,Sin(pPlayer->at3b*2));
            pPlayer->at43 = mulscale30(pPosture->at18*pPlayer->at37,Sin(pPlayer->at3b-256));
            pPlayer->at4f = mulscale30(pPosture->at1c*pPlayer->at37,Sin(pPlayer->at4b*2));
            pPlayer->at53 = mulscale30(pPosture->at20*pPlayer->at37,Sin(pPlayer->at4b-341));
            break;
    }
    pPlayer->at35a = 0;
    pPlayer->at37f = ClipLow(pPlayer->at37f-4, 0);
    pPlayer->at35e = ClipLow(pPlayer->at35e-4, 0);
    pPlayer->at362 = ClipLow(pPlayer->at362-4, 0);
    pPlayer->at366 = ClipLow(pPlayer->at366-4, 0);
    pPlayer->at36a = ClipLow(pPlayer->at36a-4, 0);
    pPlayer->at377 = ClipLow(pPlayer->at377-4, 0);
    if (pPlayer == gMe && gMe->pXSprite->health == 0)
        pPlayer->at376 = 0;
    if (!pXSprite->health)
        return;
    pPlayer->at87 = 0;
    if (pPlayer->at2f == 1)
    {
        pPlayer->at87 = 1;
        int nSector = pSprite->sectnum;
        int nSprite = gLowerLink[nSector];
        if (nSprite > 0)
        {
            if (sprite[nSprite].type == kMarker14 || sprite[nSprite].type == kMarker10)
            {
                if (pPlayer->at67 < getceilzofslope(nSector, pSprite->x, pSprite->y))
                    pPlayer->at87 = 0;
            }
        }
    }
    if (!pPlayer->at87)
    {
        pPlayer->at2f2 = 1200;
        pPlayer->at36e = 0;
        if (packItemActive(pPlayer, 1))
            packUseItem(pPlayer, 1);
    }
    int nType = kDudePlayer1-kDudeBase;
    switch (pPlayer->at2f)
    {
    case 1:
        seqSpawn(dudeInfo[nType].seqStartID+9, 3, nXSprite);
        break;
    case 2:
        seqSpawn(dudeInfo[nType].seqStartID+10, 3, nXSprite);
        break;
    default:
        if (!vel)
            seqSpawn(dudeInfo[nType].seqStartID, 3, nXSprite);
        else
            seqSpawn(dudeInfo[nType].seqStartID+8, 3, nXSprite);
        break;
    }
}

SPRITE *playerFireMissile(PLAYER *pPlayer, int a2, long a3, long a4, long a5, int a6)
{
    return actFireMissile(pPlayer->pSprite, a2, pPlayer->at6f-pPlayer->pSprite->z, a3, a4, a5, a6);
}

SPRITE * playerFireThing(PLAYER *pPlayer, int a2, int a3, int thingType, int a5)
{
    dassert(thingType >= kThingBase && thingType < kThingMax, 2636);
    return actFireThing(pPlayer->pSprite, a2, pPlayer->at6f-pPlayer->pSprite->z, pPlayer->at83+a3, thingType, a5);
}

void playerFrag(PLAYER *pKiller, PLAYER *pVictim)
{
    dassert(pKiller != NULL, 2647);
    dassert(pVictim != NULL, 2648);
    
    char buffer[128] = "";
    int nKiller = pKiller->pSprite->type-kDudePlayer1;
    dassert(nKiller >= 0 && nKiller < kMaxPlayers, 2653);
    int nVictim = pVictim->pSprite->type-kDudePlayer1;
    dassert(nVictim >= 0 && nVictim < kMaxPlayers, 2655);
    if (myconnectindex == connecthead)
    {
        sprintf(buffer, "frag %d killed %d\n", pKiller->at57+1, pVictim->at57+1);
        func_7AC28(buffer);
        buffer[0] = 0;
    }
    if (nKiller == nVictim)
    {
        pVictim->at2ee = -1;
        if (VanillaMode() || (gGameOptions.nGameType != GAMETYPE_1))
        {
            pKiller->at2c6--;
            pKiller->at2ca[nKiller]--;
        }
        if (gGameOptions.nGameType == GAMETYPE_3)
            int_21EFB0[pVictim->at2ea]--;
        int nMessage = Random(5);
        int nSound = gSuicide[nMessage].at4;
        char *pzMessage = gSuicide[nMessage].at0;
        if (pVictim == gMe && gMe->at372 <= 0)
        {
            sprintf(buffer, "You killed yourself!");
            if (gGameOptions.nGameType > GAMETYPE_0 && nSound >= 0)
                sndStartSample(nSound, 255, 2);
        }
        else if (!VanillaMode() && (gGameOptions.nGameType > GAMETYPE_0)) // use unused suicide messages for multiplayer
        {
            sprintf(buffer, gSuicide[nMessage].at0, gProfile[nVictim].name);
        }
    }
    else
    {
        if (VanillaMode() || (gGameOptions.nGameType != GAMETYPE_1))
        {
            pKiller->at2c6++;
            pKiller->at2ca[nVictim]++;
        }
        if (gGameOptions.nGameType == GAMETYPE_3)
        {
            if (pKiller->at2ea == pVictim->at2ea)
                int_21EFB0[pKiller->at2ea]--;
            else
            {
                int_21EFB0[pKiller->at2ea]++;
                int_21EFD0[pKiller->at2ea] += 120;
            }
        }
        int nMessage = Random(25);
        int nSound = gVictory[nMessage].at4;
        char *pzMessage = gVictory[nMessage].at0;
        sprintf(buffer, pzMessage, gProfile[nKiller].name, gProfile[nVictim].name);
        if (gGameOptions.nGameType > GAMETYPE_0 && nSound >= 0 && pKiller == gMe)
            sndStartSample(nSound, 255, 2);
    }
    viewSetMessage(buffer);
}

void FragPlayer(PLAYER *pPlayer, int nSprite)
{
    SPRITE *pSprite = NULL;
    if (nSprite >= 0)
        pSprite = &sprite[nSprite];
    if (pSprite && IsPlayerSprite(pSprite))
    {
        PLAYER *pKiller = &gPlayer[pSprite->type-kDudePlayer1];
        playerFrag(pKiller, pPlayer);
        int nTeam1 = pKiller->at2ea&1;
        int nTeam2 = pPlayer->at2ea&1;
        if (nTeam1 == 0)
        {
            if (nTeam1 == nTeam2)
                evSend(0, 0, 15, COMMAND_ID_3);
            else
                evSend(0, 0, 16, COMMAND_ID_3);
        }
        else
        {
            if (nTeam1 == nTeam2)
                evSend(0, 0, 16, COMMAND_ID_3);
            else
                evSend(0, 0, 15, COMMAND_ID_3);
        }
    }
}

int playerDamageArmor(PLAYER *pPlayer, DAMAGE_TYPE nType, int nDamage)
{
    DAMAGEINFO *pDamageInfo = &damageInfo[nType];
    int nArmorType = pDamageInfo->at0;
    if (nArmorType >= 0 && pPlayer->at33e[nArmorType])
    {
        int t = scale(pPlayer->at33e[nArmorType], 0, 3200, nDamage/4, (nDamage*7)/8);
        nDamage -= t;
        pPlayer->at33e[nArmorType] = ClipLow(pPlayer->at33e[nArmorType] - t, 0);
    }
    return nDamage;
}

SPRITE *func_40A94(PLAYER *pPlayer, int a2)
{
    char buffer[80];
    SPRITE *pSprite = NULL;
    switch (a2)
    {
    case 147:
        pPlayer->at90 &= ~1;
        pSprite = actDropObject(pPlayer->pSprite, 147);
        if (pSprite)
            pSprite->owner = pPlayer->at91[0];
        sprintf(buffer, "%s dropped Blue Flag", gProfile[pPlayer->at57].name);
        sndStartSample(8005, 255, 2);
        viewSetMessage(buffer);
        break;
    case 148:
        pPlayer->at90 &= ~2;
        pSprite = actDropObject(pPlayer->pSprite, 148);
        if (pSprite)
            pSprite->owner = pPlayer->at91[1];
        sprintf(buffer, "%s dropped Red Flag", gProfile[pPlayer->at57].name);
        sndStartSample(8004, 255, 2);
        viewSetMessage(buffer);
        break;
    }
    return pSprite;
}

int playerDamageSprite(int nSource, PLAYER *pPlayer, DAMAGE_TYPE nDamageType, int nDamage)
{
    dassert(nSource < kMaxSprites, 2820);
    dassert(pPlayer != NULL, 2821);
    dassert(nDamageType >= 0 && nDamageType < kDamageMax, 2822);
    if (pPlayer->ata1[nDamageType])
        return 0;
    nDamage = playerDamageArmor(pPlayer, nDamageType, nDamage);
    pPlayer->at366 = ClipHigh(pPlayer->at366+(nDamage>>3), 600);

    SPRITE *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nSprite = pSprite->index;
    int nXSprite = pSprite->extra;
    int nXSector = sector[pSprite->sectnum].extra;
    int nType = pSprite->type - kDudeBase;
    DUDEINFO *pDudeInfo = &dudeInfo[nType];
    int nDeathSeqID = -1;
    int v18 = -1;
    BOOL va = playerSeqPlaying(pPlayer, 16);
    if (!pXSprite->health)
    {
        if (va)
        {
            switch (nDamageType)
            {
            case kDamageSpirit:
                nDeathSeqID = 18;
                sfxPlay3DSound(pSprite, 716, 0);
                break;
            case kDamageExplode:
                GibSprite(pSprite, GIBTYPE_7);
                GibSprite(pSprite, GIBTYPE_15);
                pPlayer->pSprite->cstat |= kSpriteStat31;
                nDeathSeqID = 17;
                break;
            default:
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                CGibPosition gibPos(pSprite->x, pSprite->y, top);
                CGibVelocity gibVel(xvel[pSprite->index]>>1, yvel[pSprite->index]>>1, -0xccccc);
                GibSprite(pSprite, GIBTYPE_27, &gibPos, &gibVel);
                GibSprite(pSprite, GIBTYPE_7);
                fxSpawnBlood(pSprite, nDamage<<4);
                fxSpawnBlood(pSprite, nDamage<<4);
                nDeathSeqID = 17;
                break;
            }
            }
        }
    }
    else
    {
        int nHealth = pXSprite->health-nDamage;
        pXSprite->health = ClipLow(nHealth, 0);
        if (pXSprite->health > 0 && pXSprite->health < 16)
        {
            pXSprite->health = 0;
            nDamageType = kDamageBullet;
            nHealth = -25;
        }
        if (pXSprite->health > 0)
        {
            DAMAGEINFO *pDamageInfo = &damageInfo[nDamageType];
            int nSound;
            if (nDamage >= (10<<4))
                nSound = pDamageInfo->at4[0];
            else
                nSound = pDamageInfo->at4[Random(3)];
            if (nDamageType == kDamageDrown && pXSprite->at17_6 == 1 && !pPlayer->at376)
                nSound = 714;
            sfxPlay3DSound(pSprite, nSound, 0, 6);
            return nDamage;
        }
        sfxKill3DSound(pPlayer->pSprite, -1, 441);
        if (gGameOptions.nGameType == GAMETYPE_3 && pPlayer->at90)
        {
            if (pPlayer->at90&1)
                func_40A94(pPlayer, 147);
            if (pPlayer->at90&2)
                func_40A94(pPlayer, 148);
        }
        pPlayer->at1fe = 0;
        pPlayer->at1b1 = 0;
        pPlayer->at2ee = nSource;
        pPlayer->atbd = 0;
        pPlayer->at34e = 0;
        if (nDamageType == kDamageExplode && nDamage < (9<<4))
            nDamageType = kDamageFall;
        switch (nDamageType)
        {
        case kDamageExplode:
            sfxPlay3DSound(pSprite, 717, 0);
            GibSprite(pSprite, GIBTYPE_7, NULL, NULL);
            GibSprite(pSprite, GIBTYPE_15, NULL, NULL);
            pPlayer->pSprite->cstat |= 32768;
            nDeathSeqID = 2;
            break;
        case kDamageBurn:
            sfxPlay3DSound(pSprite, 718, 0, 0);
            nDeathSeqID = 3;
            break;
        case kDamageDrown:
            nDeathSeqID = 1;
            break;
        default:
            if (nHealth < -20 && gGameOptions.nGameType >= GAMETYPE_2 && Chance(0x4000))
            {
                DAMAGEINFO *pDamageInfo = &damageInfo[nDamageType];
                sfxPlay3DSound(pSprite, pDamageInfo->at10[0], 0, 2);
                nDeathSeqID = 16;
                v18 = nPlayerKeelClient;
                powerupActivate(pPlayer, 28);
                pXSprite->target = nSource;
                evPost(pSprite->index, 3, 15, CALLBACK_ID_13);
            }
            else
            {
                sfxPlay3DSound(pSprite, 716, 0);
                nDeathSeqID = 1;
            }
            break;
        }
    }
    if (nDeathSeqID < 0)
        return nDamage;
    if (nDeathSeqID != 16)
    {
        powerupClear(pPlayer);
        if (nXSector > 0 && xsector[nXSector].at17_6)
            trTriggerSector(pSprite->sectnum, &xsector[nXSector], 43);
        pSprite->flags |= 7;
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (gPlayer[p].at2ee == nSprite && gPlayer[p].at1fe > 0)
                gPlayer[p].at2ee = -1;
        }
        FragPlayer(pPlayer, nSource);
        trTriggerSprite(nSprite, pXSprite, 0);
    }
    dassert(gSysRes.Lookup(pDudeInfo->seqStartID + nDeathSeqID, "SEQ") != NULL, 3030);
    seqSpawn(pDudeInfo->seqStartID+nDeathSeqID, 3, nXSprite, v18);
    return nDamage;
}

int UseAmmo(PLAYER *pPlayer, int nAmmoType, int nDec)
{
    if (gInfiniteAmmo)
        return 9999;
    if (nAmmoType == -1)
        return 9999;
    pPlayer->at181[nAmmoType] = ClipLow(pPlayer->at181[nAmmoType]-nDec, 0);
    return pPlayer->at181[nAmmoType];
}

void playerVoodooTarget(PLAYER *pPlayer)
{
    int v4 = pPlayer->at1be.dz;
    int dz = pPlayer->at6f-pPlayer->pSprite->z;
    if (UseAmmo(pPlayer, 9, 0) < 8)
    {
        pPlayer->at34e = 0;
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        int ang = (pPlayer->at352+pPlayer->at356)&2047;
        int dx = Cos(ang) >> 16;
        int dy = Sin(ang) >> 16;
        actFireVector(pPlayer->pSprite, 0, dz, dx, dy, v4, kVectorVoodoo);
        ang = (pPlayer->at352+2048-pPlayer->at356)&2047;
        dx = Cos(ang) >> 16;
        dy = Sin(ang) >> 16;
        actFireVector(pPlayer->pSprite, 0, dz, dx, dy, v4, kVectorVoodoo);
        pPlayer->at356 += 5;
    }
    pPlayer->at34e = ClipLow(pPlayer->at34e-1, 0);
}

void playerLandingSound(PLAYER *pPlayer)
{
    static int surfaceSound[] = {
        -1,
        600,
        601,
        602,
        603,
        604,
        605,
        605,
        605,
        600,
        605,
        605,
        605,
        604,
        603
    };
    SPRITE *pSprite = pPlayer->pSprite;
    SPRITEHIT *pHit = &gSpriteHit[pSprite->extra];
    if (pHit->florhit)
    {
        char nSurf = tileGetSurfType(pHit->florhit);
        if (nSurf != 0)
            sfxPlay3DSound(pSprite, surfaceSound[nSurf], -1, 0);
    }
}

static void PlayerSurvive(int, int nXSprite)
{
    char buffer[80];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    actHealDude(pXSprite, 1, 2);
    if (gGameOptions.nGameType > GAMETYPE_0 && numplayers > 1)
    {
        sfxPlay3DSound(pSprite, 3009, 0, 6);
        if (IsPlayerSprite(pSprite))
        {
            PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
            if (pPlayer == gMe)
                viewSetMessage("I LIVE...AGAIN!!");
            else
            {
                sprintf(buffer, "%s lives again!", gProfile[pPlayer->at57].name);
                viewSetMessage(buffer);
            }
            pPlayer->atc.newWeapon = 1;
        }
    }
}

static void PlayerKeelsOver(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (gPlayer[p].pXSprite == pXSprite)
        {
            PLAYER *pPlayer = &gPlayer[p];
            playerDamageSprite(pPlayer->at2ee, pPlayer, kDamageSpirit, 500<<4);
            return;
        }
    }
}

class PlayerLoadSave : public LoadSave
{
public:
    virtual void Load(void);
    virtual void Save(void);
};

void PlayerLoadSave::Load(void)
{
    Read(int_21EFB0, sizeof(int_21EFB0));
    Read(&gNetPlayers, sizeof(gNetPlayers));
    Read(&gProfile, sizeof(gProfile));
    Read(&gPlayer, sizeof(gPlayer));
    for (int i = 0; i < gNetPlayers; i++)
    {
        gPlayer[i].pSprite = &sprite[gPlayer[i].at5b];
        gPlayer[i].pXSprite = &xsprite[gPlayer[i].pSprite->extra];
        gPlayer[i].pDudeInfo = &dudeInfo[gPlayer[i].pSprite->type-kDudeBase];
    }
}

void PlayerLoadSave::Save(void)
{
    Write(int_21EFB0, sizeof(int_21EFB0));
    Write(&gNetPlayers, sizeof(gNetPlayers));
    Write(&gProfile, sizeof(gProfile));
    Write(&gPlayer, sizeof(gPlayer));
}

static PlayerLoadSave myLoadSave;
