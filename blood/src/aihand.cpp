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
#include "aihand.h"
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

static void HandJumpSeqCallback(int, int);
static void thinkSearch(SPRITE *, XSPRITE *);
static void thinkGoto(SPRITE *, XSPRITE *);
static void thinkChase(SPRITE *, XSPRITE *);

static int nJumpClient = seqRegisterClient(HandJumpSeqCallback);

AISTATE handIdle = { 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE hand13A3B4 = { 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE handSearch = { 6, -1, 600, NULL, aiMoveForward, thinkSearch, &handIdle };
AISTATE handChase = { 6, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE handRecoil = { 5, -1, 0, NULL, NULL, NULL, &handSearch };
AISTATE handGoto = { 6, -1, 1800, NULL, aiMoveForward, thinkGoto, &handIdle };
AISTATE handJump = { 7, nJumpClient, 120, NULL, NULL, NULL, &handChase };

static void HandJumpSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    SPRITE *pSprite = &sprite[nSprite];
    SPRITE *pTarget = &sprite[pXSprite->target];
    if (IsPlayerSprite(pTarget))
    {
        PLAYER *pPlayer = &gPlayer[pTarget->type-kDudePlayer1];
        if (pPlayer->at376 == 0)
        {
            pPlayer->at376 = 1;
            actPostSprite(pSprite->index, kStatFree);
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
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 166);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->at20_0-pSprite->x;
    int dy = pXSprite->at24_0-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->at1b)
        aiNewState(pSprite, pXSprite, &handSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(SPRITE *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &handGoto);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 203);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites, 206);
    SPRITE *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &handSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], 13) > 0)
    {
        aiNewState(pSprite, pXSprite, &handSearch);
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
                if (nDist < 0x233 && klabs(nDeltaAngle) < 85 && gGameOptions.nGameType == GAMETYPE_0)
                    aiNewState(pSprite, pXSprite, &handJump);
                return;
            }
        }
    }

    aiNewState(pSprite, pXSprite, &handGoto);
    pXSprite->target = -1;
}
