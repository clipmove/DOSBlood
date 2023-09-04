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
#include "ai.h"
#include "aigilbst.h"
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

static void GillBiteSeqCallback(int, int);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);
static void thinkSwimGoto(SPRITE *, XSPRITE *);
static void thinkSwimChase(SPRITE *, XSPRITE *);
static void func_6CB00(SPRITE *, XSPRITE *);
static void func_6CD74(SPRITE *, XSPRITE *);
static void func_6D03C(SPRITE *, XSPRITE *);

static int nGillBiteClient = seqRegisterClient(GillBiteSeqCallback);

AISTATE gillBeastIdle = { 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastChase = { 9, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE gillBeastDodge = { 9, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastChase };
AISTATE gillBeastGoto = { 9, -1, 600, NULL, aiMoveForward, thinkGoto, &gillBeastIdle };
AISTATE gillBeastBite = { 6, nGillBiteClient, 120, NULL, NULL, NULL, &gillBeastChase };
AISTATE gillBeastSearch = { 9, -1, 120, NULL, aiMoveForward, thinkSearch, &gillBeastIdle };
AISTATE gillBeastRecoil = { 5, -1, 0, NULL, NULL, NULL, &gillBeastDodge };
AISTATE gillBeastSwimIdle = { 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastSwimChase = { 10, -1, 0, NULL, func_6CB00, thinkSwimChase, NULL };
AISTATE gillBeastSwimDodge = { 10, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastSwimChase };
AISTATE gillBeastSwimGoto = { 10, -1, 600, NULL, aiMoveForward, thinkSwimGoto, &gillBeastSwimIdle };
AISTATE gillBeastSwimSearch = { 10, -1, 120, NULL, aiMoveForward, thinkSearch, &gillBeastSwimIdle };
AISTATE gillBeastSwimBite = { 7, nGillBiteClient, 0, NULL, NULL, thinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeastSwimRecoil = { 5, -1, 0, NULL, NULL, NULL, &gillBeastSwimDodge };
AISTATE gillBeast13A138 = { 10, -1, 120, NULL, func_6CD74, thinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A154 = { 10, -1, 0, NULL, func_6D03C, thinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A170 = { 10, -1, 120, NULL, NULL, aiMoveTurn, &gillBeastSwimChase };

static void GillBiteSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    int dz = pSprite->z-pTarget->z;
    dx += Random3(2000);
    dy += Random3(2000);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorGillbeastBite);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorGillbeastBite);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorGillbeastBite);
}

static void thinkSearch(SPRITE *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->at16_0);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 150);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
    {
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &gillBeastSearch);
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &gillBeastSearch);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 236);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 239);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &gillBeastSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &gillBeastSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->atb*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->at17 && klabs(nDeltaAngle) <= pDudeInfo->at1b)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int nXSprite = sprite[pXSprite->reference].extra;
                gDudeSlope[nXSprite] = divscale(pTarget->z-pSprite->z, nDist, 10);
                if (nDist < 921 && klabs(nDeltaAngle) < 28)
                {
                    XSECTOR *pXSector;
                    int nXSector = sector[pSprite->sectnum].extra;
                    if (nXSector > 0)
                        pXSector = &xsector[nXSector];
                    else
                        pXSector = NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        if (pXSector && pXSector->at13_4)
                            aiNewState(pSprite, pXSprite, &gillBeastSwimBite);
                        else
                            aiNewState(pSprite, pXSprite, &gillBeastBite);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                        {
                            if (pXSector && pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &gillBeastSwimBite);
                            else
                                aiNewState(pSprite, pXSprite, &gillBeastBite);
                        }
                        else
                        {
                            if (pXSector && pXSector->at13_4)
                                aiNewState(pSprite, pXSprite, &gillBeastSwimDodge);
                            else
                                aiNewState(pSprite, pXSprite, &gillBeastDodge);
                        }
                        break;
                    default:
                        if (pXSector && pXSector->at13_4)
                            aiNewState(pSprite, pXSprite, &gillBeastSwimBite);
                        else
                            aiNewState(pSprite, pXSprite, &gillBeastBite);
                        break;
                    }
                }
            }
            return;
        }
    }

    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    if (pXSector && pXSector->at13_4)
        aiNewState(pSprite, pXSprite, &gillBeastSwimGoto);
    else
        aiNewState(pSprite, pXSprite, &gillBeastGoto);
    sfxPlay3DSound(pSprite, 1701, -1, 0);
    pXSprite->target = -1;
}

static void thinkSwimGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 440);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkSwimChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 478);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 481);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = pDudeInfo->atb+pSprite->z;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->at17 && klabs(nDeltaAngle) <= pDudeInfo->at1b)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &gillBeastSwimBite);
                else
                {
                    aiPlay3DSound(pSprite, 1700, AI_SFX_PRIORITY_1, -1);
                    aiNewState(pSprite, pXSprite, &gillBeast13A154);
                }
            }
        }
        else
            aiNewState(pSprite, pXSprite, &gillBeast13A154);
        return;
    }
    aiNewState(pSprite, pXSprite, &gillBeastSwimGoto);
    pXSprite->target = -1;
}

static void func_6CB00(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 636);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->at38-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (klabs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Random(64) < 32 && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>2;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void func_6CD74(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 706);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + dudeInfo[pSprite->type - kDudeBase].atb;
    int z2 = pTarget->z + dudeInfo[pTarget->type - kDudeBase].atb;
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->at38-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (klabs(nAng) > 341)
    {
        pXSprite->at16_0 = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int dz = z2 - z;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -dz;
}

static void func_6D03C(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 776);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    SPRITE *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + dudeInfo[pSprite->type - kDudeBase].atb;
    int z2 = pTarget->z + dudeInfo[pTarget->type - kDudeBase].atb;
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->at38-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (klabs(nAng) > 341)
    {
        pSprite->ang = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int dz = (z2 - z)<<3;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = dz;
}
