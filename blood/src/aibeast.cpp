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
#include "typedefs.h"
#include "actor.h"
#include "ai.h"
#include "aibeast.h"
#include "build.h"
#include "debug4g.h"
#include "db.h"
#include "dude.h"
#include "gameutil.h"
#include "levels.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void SlashSeqCallback(int, int);
static void StompSeqCallback(int, int);
static void MorphToBeast(SPRITE *, XSPRITE *);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);
static void thinkSwimGoto(SPRITE *, XSPRITE *);
static void thinkSwimChase(SPRITE *, XSPRITE *);
static void MoveForward(SPRITE *, XSPRITE *);
static void func_628A0(SPRITE *, XSPRITE *);
static void func_62AE0(SPRITE *, XSPRITE *);
static void func_62D7C(SPRITE *, XSPRITE *);

static int nSlashClient = seqRegisterClient(SlashSeqCallback);
static int nStompClient = seqRegisterClient(StompSeqCallback);

AISTATE beastIdle = { 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastChase = { 8, -1, 0, NULL, MoveForward, thinkChase, NULL };
AISTATE beastDodge = { 8, -1, 60, NULL, aiMoveDodge, NULL, &beastChase };
AISTATE beastGoto = { 8, -1, 600, NULL, MoveForward, thinkGoto, &beastIdle };
AISTATE beastSlash = { 6, nSlashClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastStomp = { 7, nStompClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastSearch = { 8, -1, 120, NULL, MoveForward, thinkSearch, &beastIdle };
AISTATE beastRecoil = { 5, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastTeslaRecoil = { 4, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastSwimIdle = { 9, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastSwimChase = { 9, -1, 0, NULL, func_628A0, thinkSwimChase, NULL };
AISTATE beastSwimDodge = { 9, -1, 90, NULL, aiMoveDodge, NULL, &beastSwimChase };
AISTATE beastSwimGoto = { 9, -1, 600, NULL, MoveForward, thinkSwimGoto, &beastSwimIdle };
AISTATE beastSwimSearch = { 9, -1, 120, NULL, MoveForward, thinkSearch, &beastSwimIdle };
AISTATE beastSwimSlash = { 9, nSlashClient, 0, NULL, NULL, thinkSwimChase, &beastSwimChase };
AISTATE beastSwimRecoil = { 5, -1, 0, NULL, NULL, NULL, &beastSwimDodge };
AISTATE beastMorphToBeast = { -1, -1, 0, MorphToBeast, NULL, NULL, &beastIdle };
AISTATE beastMorphFromCultist = { 2576, -1, 0, NULL, NULL, NULL, &beastMorphToBeast };
AISTATE beast138FB4 = { 9, -1, 120, NULL, func_62AE0, thinkSwimChase, &beastSwimChase };
AISTATE beast138FD0 = { 9, -1, 0, NULL, func_62D7C, thinkSwimChase, &beastSwimChase };
AISTATE beast138FEC = { 9, -1, 120, NULL, aiMoveTurn, NULL, &beastSwimChase };

static void SlashSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    // Correct ?
    int dz = pSprite->z-pTarget->z;
    dx += Random3(4000-700*gGameOptions.nDifficulty);
    dy += Random3(4000-700*gGameOptions.nDifficulty);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSlash);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSlash);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSlash);
    sfxPlay3DSound(pSprite, 9012+Random(2), -1, 0);
}

static void StompSeqCallback(int, int nXSprite)
{
    byte vb8[(kMaxSectors+7)>>3];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    SPRITE *pSprite = &sprite[pXSprite->reference];
    int nSprite = pXSprite->reference;
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector = pSprite->sectnum;
    int vc = 400;
    int v1c = 5+2*gGameOptions.nDifficulty;
    int v10 = 25+30*gGameOptions.nDifficulty;
    gAffectedSectors[0] = -1;
    gAffectedXWalls[0] = -1;
    GetClosestSpriteSectors(nSector, x, y, vc, gAffectedSectors, vb8, gAffectedXWalls);
    BOOL v4 = 0;
    int v34 = -1;
    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
    actHitcodeToData(hit, &gHitInfo, &v34);
    if (hit == 3 && v34 >= 0)
    {
        if (sprite[v34].statnum == 6)
            v4 = 0;
    }
    vc <<= 4;
    for (int nSprite2 = headspritestat[6]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
    {
        if (nSprite2 == nSprite && !v4)
            continue;
        SPRITE *pSprite2 = &sprite[nSprite2];
        if (pSprite2->extra <= 0 || pSprite2->extra >= kMaxXSprites)
            continue;
        if (pSprite2->type == 251)
            continue;
        if (pSprite2->flags&kSpriteFlag5)
            continue;
        if (!TestBitString(vb8, pSprite2->sectnum))
            continue;
        if (CheckProximity(pSprite2, x, y, z, nSector, vc))
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (klabs(bottom-sector[nSector].floorz) == 0)
            {
                int dx = klabs(pSprite->x-pSprite2->x);
                int dy = klabs(pSprite->y-pSprite2->y);
                int nDist2 = ksqrt(dx*dx + dy*dy);
                if (nDist2 <= vc)
                {
                    int nDamage = nDist2 == 0 ? v1c + v10 : v1c + ((vc-nDist2)*v10)/vc;
                    if (IsPlayerSprite(pSprite2))
                        gPlayer[pSprite2->type-kDudePlayer1].at37f += nDamage*4;
                    actDamageSprite(nSprite, pSprite2, kDamageFall, nDamage<<4);
                }
            }
        }
    }
    for (nSprite2 = headspritestat[4]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
    {
        SPRITE *pSprite2 = &sprite[nSprite2];
        if (pSprite2->flags&kSpriteFlag5)
            continue;
        if (!TestBitString(vb8, pSprite2->sectnum))
            continue;
        if (CheckProximity(pSprite2, x, y, z, nSector, vc))
        {
            XSPRITE *pXSprite = &xsprite[pSprite2->extra];
            if (pXSprite->at17_5)
                continue;
            int dx = klabs(pSprite->x-pSprite2->x);
            int dy = klabs(pSprite->y-pSprite2->y);
            int nDist2 = ksqrt(dx*dx + dy*dy);
            if (nDist2 <= vc)
            {
                int nDamage = nDist2 == 0 ? v1c + v10 : v1c + ((vc-nDist2)*v10)/vc;
                actDamageSprite(nSprite, pSprite2, kDamageFall, nDamage<<4);
            }
        }
    }
    sfxPlay3DSound(pSprite, 9015+Random(2));
}

static void MorphToBeast(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nType = 51;
    DUDEINFO *pDudeInfo = &dudeInfo[nType];
    actHealDude(pXSprite, pDudeInfo->at2, pDudeInfo->at2);
    pSprite->type = 251;
}

static void thinkSearch(SPRITE *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->at16_0);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 330);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    SECTOR *pSector = &sector[pSprite->sectnum];
    XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
    {
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        SECTOR *pSector = &sector[pSprite->sectnum];
        XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
        return;
    }
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 416);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 419);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    dx = pTarget->x-pSprite->x;
    dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        SECTOR *pSector = &sector[pSprite->sectnum];
        XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
        return;
    }
    if (IsPlayerSprite(pTarget))
    {
        PLAYER *pPlayer = &gPlayer[pTarget->type-kDudePlayer1];
        if (powerupCheck(pPlayer, 13) > 0)
        {
            SECTOR *pSector = &sector[pSprite->sectnum];
            XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
            if (pXSector && pXSector->at13_4)
                aiNewState(pSprite, pXSprite, &beastSwimSearch);
            else
                aiNewState(pSprite, pXSprite, &beastSearch);
            return;
        }
    }
    nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nAngle = getangle(dx, dy);
        int nDeltaAngle = ((nAngle+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->atb*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->at17 && klabs(nDeltaAngle) <= pDudeInfo->at1b)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int nXSprite = sprite[pXSprite->reference].extra;
                gDudeSlope[nXSprite] = divscale(pTarget->z-pSprite->z, nDist, 10);
                if (nDist < 0x1400 && nDist > 0xa00 && klabs(nDeltaAngle) < 85 && (pTarget->flags&kSpriteFlag1)
                    && IsPlayerSprite(pTarget) && Chance(0x8000))
                {
                    SECTOR *pSector = &sector[pSprite->sectnum];
                    XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    if (pXTarget->health > gPlayerTemplate[0].at2/2)
                    {
                        switch (hit)
                        {
                        case -1:
                            if (!pXSector || !pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &beastStomp);
                            break;
                        case 3:
                            if (sprite[gHitInfo.hitsprite].type != pSprite->type)
                            {
                                if (!pXSector || !pXSector->at13_4)
                                    aiNewState(pSprite, pXSprite, &beastStomp);
                            }
                            else
                            {
                                if (pXSector && pXSector->at13_4)
                                    aiNewState(pSprite, pXSprite, &beastSwimDodge);
                                else
                                    aiNewState(pSprite, pXSprite, &beastDodge);
                            }
                            break;
                        default:
                            if (!pXSector || !pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &beastStomp);
                            break;
                        }
                    }
                }
                if (nDist < 921 && klabs(nDeltaAngle) < 28)
                {
                    SECTOR *pSector = &sector[pSprite->sectnum];
                    XSECTOR *pXSector = pSector->extra > 0 ? &xsector[pSector->extra] : NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        if (pXSector && pXSector->at13_4)
                            aiNewState(pSprite, pXSprite, &beastSwimSlash);
                        else
                            aiNewState(pSprite, pXSprite, &beastSlash);
                        break;
                    case 3:
                        if (sprite[gHitInfo.hitsprite].type != pSprite->type)
                        {
                            if (pXSector && pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &beastSwimSlash);
                            else
                                aiNewState(pSprite, pXSprite, &beastSlash);
                        }
                        else
                        {
                            if (pXSector && pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &beastSwimDodge);
                            else
                                aiNewState(pSprite, pXSprite, &beastDodge);
                        }
                        break;
                    default:
                        if (pXSector && pXSector->at13_4)
                            aiNewState(pSprite, pXSprite, &beastSwimSlash);
                        else
                            aiNewState(pSprite, pXSprite, &beastSlash);
                        break;
                    }
                }
            }
            return;
        }
    }
    
    int nXSector = sector[pSprite->sectnum].extra;
    XSECTOR *pXSector = nXSector > 0 ? &xsector[nXSector] : NULL;
    if (pXSector && pXSector->at13_4)
        aiNewState(pSprite, pXSprite, &beastSwimGoto);
    else
        aiNewState(pSprite, pXSprite, &beastGoto);
    pXSprite->target = -1;
}

static void thinkSwimGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 669);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &beastSwimSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkSwimChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &beastSwimGoto);
        return;
    }
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 707);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 710);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    dx = pTarget->x-pSprite->x;
    dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &beastSwimSearch);
        return;
    }
    if (IsPlayerSprite(pTarget))
    {
        PLAYER *pPlayer = &gPlayer[pTarget->type-kDudePlayer1];
        if (powerupCheck(pPlayer, 13) > 0)
        {
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
            return;
        }
    }
    nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nAngle = getangle(dx,dy);
        int nDeltaAngle = ((nAngle+1024-pSprite->ang)&2047)-1024;
        int height = pDudeInfo->atb+pSprite->z;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->at17 && klabs(nDeltaAngle) <= pDudeInfo->at1b)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int x = pSprite->x;
                int y = pSprite->y;
                int floorZ = getflorzofslope(pSprite->sectnum, x, y);
                if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &beastSwimSlash);
                else
                {
                    aiPlay3DSound(pSprite, 9009+Random(2), AI_SFX_PRIORITY_1, -1);
                    aiNewState(pSprite, pXSprite, &beast138FD0);
                }
            }
        }
        else
            aiNewState(pSprite, pXSprite, &beast138FD0);
        return;
    }
    aiNewState(pSprite, pXSprite, &beastSwimGoto);
    pXSprite->target = -1;
}

static void MoveForward(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 865);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (klabs(nAng) > 341)
        return;
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (nDist <= 0x400 && Random(64) < 32)
        return;
    xvel[nSprite] += mulscale30(pDudeInfo->at38, Cos(pSprite->ang));
    yvel[nSprite] += mulscale30(pDudeInfo->at38, Sin(pSprite->ang));
}

static void func_628A0(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 902);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (Random(64) < 32 && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>2;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void func_62AE0(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, dz, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 965);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + dudeInfo[pSprite->type - kDudeBase].atb;
    int z2 = pTarget->z + dudeInfo[pTarget->type - kDudeBase].atb;
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pXSprite->at16_0 = (short)((pSprite->ang+512)&2047);
        return;
    }
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    dz = z2 - z;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    t1 += nAccel;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -dz;
}

static void func_62D7C(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, dz, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 1028);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + dudeInfo[pSprite->type - kDudeBase].atb;
    int z2 = pTarget->z + dudeInfo[pTarget->type - kDudeBase].atb;
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pSprite->ang = (pSprite->ang+512)&2047;
        return;
    }
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    dz = (z2 - z)<<3;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = dz;
}
