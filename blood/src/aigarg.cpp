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
#include "aigarg.h"
#include "build.h"
#include "debug4g.h"
#include "db.h"
#include "dude.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void SlashFSeqCallback(int, int);
static void ThrowFSeqCallback(int, int);
static void BlastSSeqCallback(int, int);
static void ThrowSSeqCallback(int, int);
static void thinkTarget(SPRITE *, XSPRITE *);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void MoveDodgeUp(SPRITE *, XSPRITE *);
static void MoveDodgeDown(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);
static void entryFStatue(SPRITE *, XSPRITE *);
static void entrySStatue(SPRITE *, XSPRITE *);
static void MoveForward(SPRITE *, XSPRITE *);
static void MoveSlow(SPRITE *, XSPRITE *);
static void MoveSwoop(SPRITE *, XSPRITE *);
static void MoveFly(SPRITE *, XSPRITE *);

static int nSlashFClient = seqRegisterClient(SlashFSeqCallback);
static int nThrowFClient = seqRegisterClient(ThrowFSeqCallback);
static int nThrowSClient = seqRegisterClient(ThrowSSeqCallback);
static int nBlastSClient = seqRegisterClient(BlastSSeqCallback);

AISTATE gargoyleFIdle = { 0, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE gargoyleStatueIdle = { 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE gargoyleFChase = { 0, -1, 0, NULL, MoveForward, thinkChase, &gargoyleFIdle };
AISTATE gargoyleFGoto = { 0, -1, 600, NULL, MoveForward, thinkGoto, &gargoyleFIdle };
AISTATE gargoyleFSlash = { 6, nSlashFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFThrow = { 6, nThrowFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleSThrow = { 6, nThrowSClient, 120, NULL, MoveForward, NULL, &gargoyleFChase };
AISTATE gargoyleSBlast = { 7, nBlastSClient, 60, NULL, MoveSlow, NULL, &gargoyleFChase };
AISTATE gargoyleFRecoil = { 5, -1, 0, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFSearch = { 0, -1, 120, NULL, MoveForward, thinkSearch, &gargoyleFIdle };
AISTATE gargoyleFMorph2 = { -1, -1, 0, entryFStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleFMorph = { 6, -1, 0, NULL, NULL, NULL, &gargoyleFMorph2 };
AISTATE gargoyleSMorph2 = { -1, -1, 0, entrySStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleSMorph = { 6, -1, 0, NULL, NULL, NULL, &gargoyleSMorph2 };
AISTATE gargoyleSwoop = { 0, -1, 120, NULL, MoveSwoop, thinkChase, &gargoyleFChase };
AISTATE gargoyleFly = { 0, -1, 120, NULL, MoveFly, thinkChase, &gargoyleFChase };
AISTATE gargoyleTurn = { 0, -1, 120, NULL, aiMoveTurn, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeUp = { 0, -1, 60, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpRight = { 0, -1, 90, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpLeft = { 0, -1, 90, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeDown = { 0, -1, 120, NULL, MoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownRight = { 0, -1, 90, NULL, MoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownLeft = { 0, -1, 90, NULL, MoveDodgeDown, NULL, &gargoyleFChase };

static void SlashFSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    DUDEINFO *pDudeInfoT = &dudeInfo[pTarget->type - kDudeBase];
    int height = (pSprite->yrepeat*pDudeInfo->atb)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->atb)<<2;
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    int dz = height-height2;
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSlash);
    actFireVector(pSprite, 0, 0, dx+Random(50), dy-Random(50), dz, kVectorSlash);
    actFireVector(pSprite, 0, 0, dx-Random(50), dy+Random(50), dz, kVectorSlash);
}

static void ThrowFSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    actFireThing(pSprite, 0, 0, gDudeSlope[nXSprite]-7500, 421, 0xeeeee);
}

static void BlastSSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    BOOL hackvar = Chance(1234);
    SPRITE *pTarget = &sprite[pXSprite->target];
    int height = (dudeInfo[pSprite->type-kDudeBase].atb*pSprite->yrepeat) << 2;

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
        if (pSprite2 == pSprite)
            continue;
        if (!(pSprite2->flags&kSpriteFlag3))
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
        if (bottom<tz-tsr || top>tz+tsr)
            continue;
        int nDist2 = Dist3d(tx-x2,ty-y2,tz-z2);
        if (nDist2 >= nClosest)
            continue;
        int nAngle = getangle(x2-x, y2-y);
        if (klabs(((nAngle-pSprite->ang+1024)&2047)-1024) > tt.at8)
            continue;
        int dzCenter = pSprite2->z-pSprite->z;
        if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
        {
            nClosest = nDist2;
            aim.dx = Cos(nAngle)>>16;
            aim.dy = Sin(nAngle)>>16;
            aim.dz = divscale(dzCenter, nDist, 10);
            if (dzCenter > -0x333)
                aim.dz = divscale(dzCenter, nDist, 10);
            else if (dzCenter < -0x333 && dzCenter > -0xb33)
                aim.dz = divscale(dzCenter, nDist, 10)+9460;
            else if (dzCenter < -0xb33 && dzCenter > -0x3000)
                aim.dz = divscale(dzCenter, nDist, 10)+9460;
            else if (dzCenter < -0x3000)
                aim.dz = divscale(dzCenter, nDist, 10)-7500;
            else
                aim.dz = divscale(dzCenter, nDist, 10);
        }
        else
            aim.dz = divscale(dzCenter, nDist, 10);
    }
    if (IsPlayerSprite(pTarget))
    {
        actFireMissile(pSprite, -120, 0, aim.dx, aim.dy, aim.dz, 311);
        actFireMissile(pSprite, 120, 0, aim.dx, aim.dy, aim.dz, 311);
    }
}

static void ThrowSSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    actFireThing(pSprite, 0, 0, gDudeSlope[nXSprite]-7500, 421, Chance(0x6000) ? 0x133333 : 0x111111);
}

static void thinkTarget(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 358);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    DUDEEXTRA_GARGOYLE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.gargoyle;
    if (pDudeExtraE->at8 && pDudeExtraE->at4 < 10)
        pDudeExtraE->at4++;
    else if (pDudeExtraE->at4 >= 10 && pDudeExtraE->at8)
    {
        pXSprite->at16_0 += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(pSprite, pXSprite, &gargoyleTurn);
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
                int nDeltaAngle = ((1024+nAngle-pSprite->ang)&2047)-1024;
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
    func_5F15C(pSprite, pXSprite);
}

static void thinkGoto(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 453);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void MoveDodgeUp(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 625);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
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
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 665);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->at17_3 == 0)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
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
        aiNewState(pSprite, pXSprite, &gargoyleFGoto);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 709);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 712);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->at17)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
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
                int floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                switch (pSprite->type)
                {
                case 206:
                    if (nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1408, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != 207)
                            {
                                sfxPlay3DSound(pSprite, 1408, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1408, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1406, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != 207)
                            {
                                sfxPlay3DSound(pSprite, 1406, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1406, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0xa00)
                    {
                        aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                        aiNewState(pSprite, pXSprite, &gargoyleSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                    break;
                case 207:
                    if (nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1457, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != 206)
                            {
                                sfxPlay3DSound(pSprite, 1457, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1457, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != 206)
                                aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
                    {
                        if (pSprite->type == 206)
                            aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                        else
                            aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
                        aiNewState(pSprite, pXSprite, &gargoyleSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
                    break;
                }
            }
            return;
        }
        else
        {
            aiNewState(pSprite, pXSprite, &gargoyleFly);
            return;
        }
    }

    aiNewState(pSprite, pXSprite, &gargoyleFGoto);
    pXSprite->target = -1;
}

static void entryFStatue(SPRITE *pSprite, XSPRITE *pXSprite)
{
    DUDEINFO *pDudeInfo = &dudeInfo[6];
    actHealDude(pXSprite, pDudeInfo->at2, pDudeInfo->at2);
    pSprite->type = 206;
}

static void entrySStatue(SPRITE *pSprite, XSPRITE *pXSprite)
{
    DUDEINFO *pDudeInfo = &dudeInfo[7];
    actHealDude(pXSprite, pDudeInfo->at2, pDudeInfo->at2);
    pSprite->type = 207;
}

static void MoveForward(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 963);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
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
        t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void MoveSlow(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 1029);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pXSprite->at16_0 = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
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
    t1 = nAccel>>1;
    t2 >>= 1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type)
    {
    case 206:
        zvel[nSprite] = 0x44444;
        break;
    case 207:
        zvel[nSprite] = 0x35555;
        break;
    }
}

static void MoveSwoop(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 1106);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pXSprite->at16_0 = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
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
    t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type)
    {
    case 206:
        zvel[nSprite] = t1;
        break;
    case 207:
        zvel[nSprite] = t1;
        break;
    }
}

static void MoveFly(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 1182);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->at38<<2;
    if (klabs(nAng) > 341)
    {
        pSprite->ang = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
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
    switch (pSprite->type)
    {
    case 206:
        zvel[nSprite] = -t1;
        break;
    case 207:
        zvel[nSprite] = -t1;
        break;
    }
    klabs(zvel[nSprite]);
}


