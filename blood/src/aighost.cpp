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
#include "aighost.h"
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

static void SlashSeqCallback(int, int);
static void ThrowSeqCallback(int, int);
static void BlastSeqCallback(int, int);
static void thinkTarget(SPRITE *, XSPRITE *);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void MoveDodgeUp(SPRITE *, XSPRITE *);
static void MoveDodgeDown(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);
static void MoveForward(SPRITE *, XSPRITE *);
static void MoveSlow(SPRITE *, XSPRITE *);
static void MoveSwoop(SPRITE *, XSPRITE *);
static void MoveFly(SPRITE *, XSPRITE *);

static int nSlashClient = seqRegisterClient(SlashSeqCallback);
static int nThrowClient = seqRegisterClient(ThrowSeqCallback);
static int nBlastClient = seqRegisterClient(BlastSeqCallback);

AISTATE ghostIdle = { 0, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE ghostChase = { 0, -1, 0, NULL, MoveForward, thinkChase, &ghostIdle };
AISTATE ghostGoto = { 0, -1, 600, NULL, MoveForward, thinkGoto, &ghostIdle };
AISTATE ghostSlash = { 6, nSlashClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostThrow = { 6, nThrowClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostBlast = { 6, nBlastClient, 120, NULL, MoveSlow, NULL, &ghostChase };
AISTATE ghostRecoil = { 5, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostTeslaRecoil = { 4, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostSearch = { 0, -1, 120, NULL, MoveForward, thinkSearch, &ghostIdle };
AISTATE ghostSwoop = { 0, -1, 120, NULL, MoveSwoop, thinkChase, &ghostChase };
AISTATE ghostFly = { 0, -1, 0, NULL, MoveFly, thinkChase, &ghostChase };
AISTATE ghostTurn = { 0, -1, 120, NULL, aiMoveTurn, NULL, &ghostChase };
AISTATE ghostDodgeUp = { 0, -1, 60, NULL, MoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpRight = { 0, -1, 90, NULL, MoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpLeft = { 0, -1, 90, NULL, MoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeDown = { 0, -1, 120, NULL, MoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownRight = { 0, -1, 90, NULL, MoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownLeft = { 0, -1, 90, NULL, MoveDodgeDown, NULL, &ghostChase };

static void SlashSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    DUDEINFO *pDudeInfoT = &dudeInfo[pTarget->type-kDudeBase];
    int height = (pSprite->yrepeat*pDudeInfo->atb)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->atb)<<2;
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    int dz = height - height2;
    sfxPlay3DSound(pSprite, 1406, 0);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_12);
    actFireVector(pSprite, 0, 0, dx+Random(50), dy-Random(50), dz, VECTOR_TYPE_12);
    actFireVector(pSprite, 0, 0, dx-Random(50), dy+Random(50), dz, VECTOR_TYPE_12);
}

static void ThrowSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE* pSprite = &sprite[nSprite];
    int slope = gDudeSlope[nXSprite] - 7500;
    actFireThing(pSprite, 0, 0, slope, 421, 0xeeeee);
}

static void BlastSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    BOOL r = Chance(1234); // ???
    SPRITE *pTarget = &sprite[pXSprite->target];
    int height = (pSprite->yrepeat*dudeInfo[pSprite->type-kDudeBase].atb) << 2;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nDist = approxDist(dx, dy);
    int nAngle = getangle(dx, dy);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = height;
    VECTOR3D aim;
    aim.dx = Cos(pSprite->ang)>>16;
    aim.dy = Sin(pSprite->ang)>>16;
    aim.dz = gDudeSlope[nXSprite];
    TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
    int nClosest = 0x7fffffff;
    for (short nSprite2 = headspritestat[6]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
    {
        SPRITE *pSprite2 = &sprite[nSprite2];
        if (pSprite == pSprite2 || !(pSprite2->flags&kSpriteFlag3))
            continue;
        int x2 = pSprite2->x;
        int y2 = pSprite2->y;
        int z2 = pSprite2->z;
        int nDist = approxDist(x2-x, y2-y);
        if (nDist == 0 || nDist > 0x2800)
            continue;
        if (tt.at10)
        {
            int t = divscale(nDist, tt.at10, 12);
            x2 += (xvel[nSprite2]*t)>>12;
            y2 += (yvel[nSprite2]*t)>>12;
            z2 += (zvel[nSprite2]*t)>>8;
        }
        int tx = x+mulscale30(Cos(pSprite->ang), nDist);
        int ty = y+mulscale30(Sin(pSprite->ang), nDist);
        int tz = z+mulscale(gDudeSlope[nXSprite], nDist, 10);
        int tsr = mulscale(9460, nDist, 10);
        int top, bottom;
        GetSpriteExtents(pSprite2, &top, &bottom);
        if (bottom < tz-tsr || top > tz+tsr)
            continue;
        int nDist2 = Dist3d(tx - x2, ty - y2, tz - z2);
        if (nDist2 >= nClosest)
            continue;
        int nAngle = getangle(x2-x, y2-y);
        if (klabs(((nAngle-pSprite->ang+1024)&2047)-1024) > tt.at8)
            continue;
        int tz2 = pSprite2->z-pSprite->z;
        if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
        {
            nClosest = nDist2;
            aim.dx = Cos(nAngle)>>16;
            aim.dy = Sin(nAngle)>>16;
            aim.dz = divscale(tz2, nDist, 10);
            if (tz2 > -0x333)
                aim.dz = divscale(tz2, nDist, 10);
            else if (tz2 < -0x333 && tz2 > -0xb33)
                aim.dz = divscale(tz2, nDist, 10)+9460;
            else if (tz2 < -0xb33 && tz2 > -0x3000)
                aim.dz = divscale(tz2, nDist, 10)+9460;
            else if (tz2 < -0x3000)
                aim.dz = divscale(tz2, nDist, 10)-7500;
            else
                aim.dz = divscale(tz2, nDist, 10);
        }
        else
            aim.dz = divscale(tz2, nDist, 10);
    }
    if (IsPlayerSprite(pTarget))
    {
        sfxPlay3DSound(pSprite, 489, 0);
        actFireMissile(pSprite, 0, 0, aim.dx, aim.dy, aim.dz, 307);
    }
}

char hackfunc7()
{
    return 1;
}

static void thinkTarget(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 315);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    DUDEEXTRA_GHOST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.ghost;
    if (pDudeExtraE->at8 && pDudeExtraE->at4 < 10)
        pDudeExtraE->at4++;
    else if (pDudeExtraE->at4 >= 10 && pDudeExtraE->at8)
    {
        pXSprite->at16_0 += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(pSprite, pXSprite, &ghostTurn);
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
                    pDudeExtraE->at4 = 0;
                    aiSetTarget(pXSprite, pPlayer->at5b);
                    aiActivateDude(pSprite, pXSprite);
                    return;
                }
                else if (nDist < pDudeInfo->at13)
                {
                    pDudeExtraE->at4 = 0;
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
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 410);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dx = pXSprite->at20_0-pSprite->x;
    dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &ghostSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void MoveDodgeUp(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 542);
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
    zvel[nSprite] = -0x1d555;
}

static void MoveDodgeDown(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 579);
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
        aiNewState(pSprite, pXSprite, &ghostGoto);
        return;
    }
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 623);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 626);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    dx = pTarget->x-pSprite->x;
    dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &ghostSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        aiNewState(pSprite, pXSprite, &ghostSearch);
        return;
    }
    nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nAngle = getangle(dx, dy);
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
                int nSector = pSprite->sectnum;
                int floorZ = getflorzofslope(nSector, x, y);
                switch (pSprite->type)
                {
                case 210:
                    if (nDist < 0x2000 && nDist > 0x1000 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (sprite[gHitInfo.hitsprite].type != pSprite->type && sprite[gHitInfo.hitsprite].type != 210)
                                aiNewState(pSprite, pXSprite, &ghostBlast);
                            break;
                        case -1:
                            aiNewState(pSprite, pXSprite, &ghostBlast);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &ghostBlast);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (sprite[gHitInfo.hitsprite].type != pSprite->type && sprite[gHitInfo.hitsprite].type != 210)
                                aiNewState(pSprite, pXSprite, &ghostSlash);
                            break;
                        case -1:
                            aiNewState(pSprite, pXSprite, &ghostSlash);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &ghostSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
                    {
                        aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1);
                        aiNewState(pSprite, pXSprite, &ghostSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1);
                    break;
                }
            }
            return;
        }
        else
        {
            aiNewState(pSprite, pXSprite, &ghostFly);
            return;
        }
    }

    aiNewState(pSprite, pXSprite, &ghostGoto);
    pXSprite->target = -1;
}

static void MoveForward(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 780);
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
        t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void MoveSlow(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 843);
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
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    int _t1 = (nAccel >> 1);
    int _t2 = (t2 >> 1);
    xvel[nSprite] = dmulscale30(_t1, nCos, _t2, nSin);
    yvel[nSprite] = dmulscale30(_t1, nSin, -_t2, nCos);
    switch (pSprite->type)
    {
    case 210:
        zvel[nSprite] = 0x44444;
        break;
    }
}

static void MoveSwoop(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 914);
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
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    int _t1 = t1 + (nAccel >> 1);
    int _t2 = t2;
    xvel[nSprite] = dmulscale30(_t1, nCos, _t2, nSin);
    yvel[nSprite] = dmulscale30(_t1, nSin, -_t2, nCos);
    switch (pSprite->type)
    {
    case 210:
        zvel[nSprite] = _t1;
        break;
    }
}

static void MoveFly(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    int dx, dy, nDist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 984);
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
    if (Chance(0x4000) && nDist <= 0x400)
        return;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    t1 += nAccel >> 1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type)
    {
    case 210:
        zvel[nSprite] = -t1;
        break;
    }
}

