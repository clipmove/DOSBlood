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
#include "aispid.h"
#include "build.h"
#include "debug4g.h"
#include "db.h"
#include "dude.h"
#include "endgame.h"
#include "gameutil.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void SpidBiteSeqCallback(int, int);
static void SpidJumpSeqCallback(int, int);
static void func_71370(int, int);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);

static int nBiteClient = seqRegisterClient(SpidBiteSeqCallback);
static int nJumpClient = seqRegisterClient(SpidJumpSeqCallback);
static int dword_279B50 = seqRegisterClient(func_71370);

AISTATE spidIdle = { 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE spidChase = { 7, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE spidDodge = { 7, -1, 90, NULL, aiMoveDodge, NULL, &spidChase };
AISTATE spidGoto = { 7, -1, 600, NULL, aiMoveForward, thinkGoto, &spidIdle };
AISTATE spidSearch = { 7, -1, 1800, NULL, aiMoveForward, thinkSearch, &spidIdle };
AISTATE spidBite = { 6, nBiteClient, 60, NULL, NULL, NULL, &spidChase };
AISTATE spidJump = { 8, nJumpClient, 60, NULL, aiMoveForward, NULL, &spidChase };
AISTATE spid13A92C = { 0, dword_279B50, 60, NULL, NULL, NULL, &spidIdle };

static char func_70D30(XSPRITE *pXDude, int a2, int a3)
{
    dassert(pXDude != NULL, 92);
    int nDude = pXDude->reference;
    SPRITE *pDude = &sprite[nDude];
    if (IsPlayerSprite(pDude))
    {
        a2 <<= 4;
        a3 <<= 4;
        if (IsPlayerSprite(pDude))
        {
            PLAYER *pPlayer = &gPlayer[pDude->type-kDudePlayer1];
            if (a3 > pPlayer->at36a)
            {
                pPlayer->at36a = ClipHigh(pPlayer->at36a+a2, a3);
                return 1;
            }
        }
    }
    return 0;
}

static void SpidBiteSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    dx += Random2(2000);
    dy += Random2(2000);
    int dz = Random2(2000);
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 142);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 145);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    if (IsPlayerSprite(pTarget))
    {
        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
        if (hit == 3)
        {
            if (sprite[gHitInfo.hitsprite].type <= kDudePlayer8 && sprite[gHitInfo.hitsprite].type >= kDudePlayer1)
            {
                dz += pTarget->z-pSprite->z;
                if (pTarget->type >= kDudePlayer1 && pTarget->type <= kDudePlayer8)
                {
                    PLAYER *pPlayer = &gPlayer[pTarget->type-kDudePlayer1];
                    switch (pSprite->type)
                    {
                    case 213:
                        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                        if (IsPlayerSprite(pTarget) && !pPlayer->at31a && powerupCheck(pPlayer, 14) <= 0
                            && Chance(0x4000))
                            powerupActivate(pPlayer, 28);
                        break;
                    case 214:
                        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                        if (Chance(0x5000))
                            func_70D30(pXTarget, 4, 16);
                        break;
                    case 215:
                        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                        func_70D30(pXTarget, 8, 16);
                        break;
                    case 216:
                    {
                        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                        dx += Random2(2000);
                        dy += Random2(2000);
                        dz += Random2(2000);
                        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                        func_70D30(pXTarget, 8, 16);
                        break;
                    }
                    }
                }
            }
        }
    }
}

static void SpidJumpSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    dx += Random2(200);
    dy += Random2(200);
    int dz = Random2(200);
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 245);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 248);
    SPRITE *pTarget = &sprite[pXSprite->target];
    if (IsPlayerSprite(pTarget))
    {
        dz += pTarget->z-pSprite->z;
        if (pTarget->type >= kDudePlayer1 && pTarget->type <= kDudePlayer8)
        {
            switch (pSprite->type)
            {
            case 213:
            case 214:
            case 215:
                xvel[nSprite] = dx << 16;
                yvel[nSprite] = dy << 16;
                zvel[nSprite] = dz << 16;
                break;
            }
        }
    }
}

static void func_71370(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 296);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 299);
    SPRITE *pTarget = &sprite[pXSprite->target];
    DUDEEXTRA_SPIDER *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.spider;
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    SPRITE *pSpawn = NULL;
    if (IsPlayerSprite(pTarget) && pDudeExtraE->at4 < 10)
    {
        if (nDist < 0x1a00 && nDist > 0x1400 && klabs(pSprite->ang-nAngle) < pDudeInfo->at1b)
            pSpawn = func_36878(pSprite, 214, pSprite->clipdist, 0);
        else if (nDist < 0x1400 && nDist > 0xc00 && klabs(pSprite->ang-nAngle) < pDudeInfo->at1b)
            pSpawn = func_36878(pSprite, 213, pSprite->clipdist, 0);
        else if (nDist < 0xc00 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
            pSpawn = func_36878(pSprite, 213, pSprite->clipdist, 0);
        if (pSpawn)
        {
            pDudeExtraE->at4++;
            pSpawn->owner = nSprite;
            gKillMgr.func_263E0(1);
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
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 354);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &spidSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &spidGoto);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 391);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 394);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &spidSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        aiNewState(pSprite, pXSprite, &spidSearch);
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
                switch (pSprite->type)
                {
                case 214:
                    if (nDist < 0x399 && klabs(nDeltaAngle) < 85)
                        aiNewState(pSprite, pXSprite, &spidBite);
                    break;
                case 213:
                case 215:
                    if (nDist < 0x733 && nDist > 0x399 && klabs(nDeltaAngle) < 85)
                        aiNewState(pSprite, pXSprite, &spidJump);
                    else if (nDist < 0x399 && klabs(nDeltaAngle) < 85)
                        aiNewState(pSprite, pXSprite, &spidBite);
                    break;
                case 216:
                    if (nDist < 0x733 && nDist > 0x399 && klabs(nDeltaAngle) < 85)
                        aiNewState(pSprite, pXSprite, &spidJump);
                    else if (Chance(0x8000))
                        aiNewState(pSprite, pXSprite, &spid13A92C);
                    break;
                }
                return;
            }
        }
    }

    aiNewState(pSprite, pXSprite, &spidGoto);
    pXSprite->target = -1;
}
