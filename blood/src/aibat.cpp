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
#include "aibat.h"
#include "build.h"
#include "debug4g.h"
#include "db.h"
#include "dude.h"
#include "gameutil.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void BiteSeqCallback(int, int);
static void thinkTarget(SPRITE *, XSPRITE *);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void thinkPonder(SPRITE *, XSPRITE *);
static void MoveDodgeUp(SPRITE *, XSPRITE *);
static void MoveDodgeDown(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);
static void MoveForward(SPRITE *, XSPRITE *);
static void MoveSwoop(SPRITE *, XSPRITE *);
static void MoveFly(SPRITE *, XSPRITE *);
static void MoveToCeil(SPRITE *, XSPRITE *);

static int nBiteClient = seqRegisterClient(BiteSeqCallback);

AISTATE batIdle = { 0, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE batFlyIdle = { 6, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE batChase = { 6, -1, 0, NULL, MoveForward, thinkChase, &batFlyIdle };
AISTATE batPonder = { 6, -1, 0, NULL, NULL, thinkPonder, NULL };
AISTATE batGoto = { 6, -1, 600, NULL, MoveForward, thinkGoto, &batFlyIdle };
AISTATE batBite = { 7, nBiteClient, 60, NULL, NULL, NULL, &batPonder };
AISTATE batRecoil = { 5, -1, 0, NULL, NULL, NULL, &batChase };
AISTATE batSearch = { 6, -1, 120, NULL, MoveForward, thinkSearch, &batFlyIdle };
AISTATE batSwoop = { 6, -1, 60, NULL, MoveSwoop, thinkChase, &batChase };
AISTATE batFly = { 6, -1, 0, NULL, MoveFly, thinkChase, &batChase };
AISTATE batTurn = { 6, -1, 60, NULL, aiMoveTurn, NULL, &batChase };
AISTATE batHide = { 6, -1, 0, NULL, MoveToCeil, MoveForward, NULL };
AISTATE batDodgeUp = { 6, -1, 120, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpRight = { 6, -1, 90, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpLeft = { 6, -1, 90, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeDown = { 6, -1, 120, NULL, MoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownRight = { 6, -1, 90, NULL, MoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownLeft = { 6, -1, 90, NULL, MoveDodgeDown, 0, &batChase };

static void BiteSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];

    int dx = Cos(pSprite->ang) >> 16;
    int dy = Sin(pSprite->ang) >> 16;
    int dz = 0;

    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 120);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    DUDEINFO *pDudeInfoT = &dudeInfo[pTarget->type-kDudeBase];
    int height = (pSprite->yrepeat*pDudeInfo->atb)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->atb)<<2;
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 126);
    dz = height2 - height;
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorBatBite);
}

static void thinkTarget(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 143);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    DUDEEXTRA_BAT *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.bat;
    if (pDudeExtraE->at8 && pDudeExtraE->at4 < 10)
        pDudeExtraE->at4++;
    else if (pDudeExtraE->at4 >= 10 && pDudeExtraE->at8)
    {
        pDudeExtraE->at4 = 0;
        pXSprite->at16_0 += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(pSprite, pXSprite, &batTurn);
        return;
    }
    if (!Chance(pDudeInfo->at33))
        return;
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        PLAYER *pPlayer = &gPlayer[p];
        if (pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, 13) > 0)
            continue;
        int x = pPlayer->pSprite->x;
        int y = pPlayer->pSprite->y;
        int z = pPlayer->pSprite->z;
        int nSector = pPlayer->pSprite->sectnum;
        int dx = x-pSprite->x;
        int dy = y-pSprite->y;
        int nDist = approxDist(dx, dy);
        if (nDist <= pDudeInfo->at17 || nDist <= pDudeInfo->at13)
        {
            int height = (pDudeInfo->atb*pSprite->yrepeat)<<2;
            if (cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z-height, pSprite->sectnum))
            {
                int nAngle = getangle(dx, dy);
                int nDeltaAngle = ((nAngle+1024-pSprite->ang)&2047)-1024;
                if (nDist < pDudeInfo->at17 && klabs(nDeltaAngle) <= pDudeInfo->at1b)
                {
                    aiSetTarget(pXSprite, pPlayer->at5b);
                    aiActivateDude(pSprite, pXSprite);
                    return;
                }
                else if (nDist < pDudeInfo->at13)
                {
                    aiSetTarget(pXSprite, x, y, z);
                    aiActivateDude(pSprite, pXSprite);
                    return;
                }
            }
        }
    }
}

static void thinkSearch(SPRITE *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->at16_0);
    thinkTarget(pSprite, pXSprite);
}

static void thinkGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 237);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &batSearch);
    thinkTarget(pSprite, pXSprite);
}

static void thinkPonder(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &batSearch);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 273);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 276);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &batSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->atb*pSprite->yrepeat)<<2;
        int height2 = (dudeInfo[pTarget->type-kDudeBase].atb*pTarget->yrepeat)<<2;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            aiSetTarget(pXSprite, pXSprite->target);
            if (height2-height < 0x3000 && nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x5000 && nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else if (height2-height < 0x2000 && nDist < 0x200 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x6000 && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else if (height2-height < 0x2000 && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height < 0x2000 && klabs(nDeltaAngle) < 85 && nDist > 0x1400)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x4000)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            return;
        }
    }
    aiNewState(pSprite, pXSprite, &batGoto);
    pXSprite->target = -1;
}

static void MoveDodgeUp(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 372);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    if (pXSprite->at17_3 > 0)
        t2 += pDudeInfo->at3c;
    else
        t2 -= pDudeInfo->at3c;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -0x52aaa;
}

static void MoveDodgeDown(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 409);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->at17_3 == 0)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    if (pXSprite->at17_3 > 0)
        t2 += pDudeInfo->at3c;
    else
        t2 -= pDudeInfo->at3c;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = 0x44444;
}

static void thinkChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &batGoto);
        return;
    }
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 452);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 455);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    dx = pTarget->x-pSprite->x;
    dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &batSearch);
        return;
    }
    if (IsPlayerSprite(pTarget))
    {
        PLAYER *pPlayer = &gPlayer[pTarget->type-kDudePlayer1];
        if (powerupCheck(pPlayer, 13) > 0)
        {
            aiNewState(pSprite, pXSprite, &batSearch);
            return;
        }
    }
    nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nAngle = getangle(dx,dy);
        int nDeltaAngle = ((nAngle+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->atb*pSprite->yrepeat)<<2;
        // Should be dudeInfo[pTarget->type-kDudeBase]
        int height2 = (pDudeInfo->atb*pTarget->yrepeat)<<2;
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
                if (height2-height < 0x2000 && nDist < 0x200 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batBite);
                else if ((height2-height > 0x5000 || floorZ-bottom > 0x5000) && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batSwoop);
                else if ((height2-height < 0x3000 || floorZ-bottom < 0x3000) && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batFly);
                return;
            }
        }
        else
        {
            aiNewState(pSprite, pXSprite, &batFly);
            return;
        }
    }

    pXSprite->target = -1;
    aiNewState(pSprite, pXSprite, &batHide);
}

static void MoveForward(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 553);
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
    if (Random(64) < 32 && nDist <= 0x200)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void MoveSwoop(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 615);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
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
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x200)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);

    t1 += nAccel>>1;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = 0x44444;
}

static void MoveFly(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 678);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pSprite->ang = (short)((pSprite->ang+512)&2047);
        return;
    }
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x200)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);

    t1 += nAccel>>1;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -0x2d555;
}

void MoveToCeil(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int x = pSprite->x;
    int y = pSprite->y;
    int nSector = pSprite->sectnum;
    if (pSprite->z - pXSprite->at28_0 < 0x1000)
    {
        DUDEEXTRA_BAT *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.bat;
        pDudeExtraE->at8 = 0;
        pSprite->flags = 0;
        aiNewState(pSprite, pXSprite, &batIdle);
    }
    else
        aiSetTarget(pXSprite, x, y, sector[nSector].ceilingz);
}
