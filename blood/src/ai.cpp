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
#include <stdlib.h>
#include <string.h>
#include "typedefs.h"
#include "actor.h"
#include "ai.h"
#include "aibat.h"
#include "aibeast.h"
#include "aiboneel.h"
#include "aiburn.h"
#include "aicaleb.h"
#include "aicerber.h"
#include "aicult.h"
#include "aigarg.h"
#include "aighost.h"
#include "aigilbst.h"
#include "aihand.h"
#include "aihound.h"
#include "aiinnoc.h"
#include "aipod.h"
#include "airat.h"
#include "aispid.h"
#include "aitchern.h"
#include "aizomba.h"
#include "aizombf.h"
#include "build.h"
#include "db.h"
#include "debug4g.h"
#include "dude.h"
#include "error.h"
#include "eventq.h"
#include "gameutil.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "misc.h"
#include "player.h"
#include "resource.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static int cumulDamage[kMaxXSprites];
int gDudeSlope[kMaxXSprites];
DUDEEXTRA gDudeExtra[kMaxXSprites];

static AISTATE genIdle = { 0, -1, 0, NULL, NULL, NULL, NULL };
static AISTATE genRecoil = { 5, -1, 20, NULL, NULL, NULL, &genIdle };

int int_138BB0[5] = {0x2000, 0x4000, 0x8000, 0xa000, 0xe000};

BOOL aiSeqPlaying(SPRITE *pSprite, int nSeq)
{
    // ???
    if (pSprite->statnum == 6 && (pSprite->type >= kDudeBase || pSprite->type < kDudeMax))
    {
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
        int t = seqGetID(3, pSprite->extra);
        if (t == pDudeInfo->seqStartID + nSeq && seqGetStatus(3, pSprite->extra) >= 0)
            return 1;
    }
    return 0;
}

void aiPlay3DSound(SPRITE *pSprite, int a2, AI_SFX_PRIORITY a3, int a4)
{
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    if (a3 == AI_SFX_PRIORITY_0)
        sfxPlay3DSound(pSprite, a2, a4, 2);
    else if (a3 > pDudeExtra->at5 || gFrameClock >= pDudeExtra->at0)
    {
        sfxKill3DSound(pSprite);
        sfxPlay3DSound(pSprite, a2, a4, 0);
        pDudeExtra->at5 = a3;
        pDudeExtra->at0 = gFrameClock+120;
    }
}

void aiNewState(SPRITE *pSprite, XSPRITE *pXSprite, AISTATE *pAIState)
{
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    pXSprite->at32_0 = pAIState->at8;
    pXSprite->at34 = pAIState;
    if (pAIState->at0 >= 0)
    {
        if (gSysRes.Lookup(pDudeInfo->seqStartID+pAIState->at0, "SEQ") == NULL)
            return;
        seqSpawn(pDudeInfo->seqStartID+pAIState->at0, 3, pSprite->extra, pAIState->at4);
    }
    if (pAIState->atc)
        pAIState->atc(pSprite, pXSprite);
}

BOOL CanMove(SPRITE *pSprite, int a2, int nAngle, int nRange)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    HitScan(pSprite, z, Cos(nAngle)>>16, Sin(nAngle)>>16, 0, CLIPMASK0, nRange);
    int nDist = approxDist(x-gHitInfo.hitx, y-gHitInfo.hity);
    if (nDist - (pSprite->clipdist << 2) < nRange)
    {
        if (gHitInfo.hitsprite >= 0 && gHitInfo.hitsprite == a2)
            return 1;
        return 0;
    }
    x += mulscale30(nRange, Cos(nAngle));
    y += mulscale30(nRange, Sin(nAngle));
    int nSector = pSprite->sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors, 342);
    if (!FindSector(x, y, z, &nSector))
        return 0;
    int floorZ = getflorzofslope(nSector, x, y);
    int ceilZ = getceilzofslope(nSector, x, y);
    BOOL vdl = 0; // Depth
    BOOL vdh = 0; // Damage
    BOOL vbl = 0; // Underwater
    BOOL vbh = 0; // Warp
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->at13_4)
            vbl = 1;
        if (pXSector->at13_5)
            vdl = 1;
        if (sector[nSector].type == 618 || pXSector->at33_1 > 0)
            vdh = 1;
    }
    int nUpper = gUpperLink[nSector];
    int nLower = gLowerLink[nSector];
    if (nUpper >= 0)
    {
        if (sprite[nUpper].type == 9 || sprite[nUpper].type == 13)
            vbh = vdl = 1;
    }
    if (nLower >= 0)
    {
        if (sprite[nLower].type == 10 || sprite[nLower].type == 14)
            vdl = 1;
    }
    switch (pSprite->type)
    {
    case 206:
    case 207:
    case 219:
        if (pSprite->clipdist > nDist)
            return 0;
        if (vdl)
        {
            // Ouch...
            if (vdl)
                return 0;
            if (vdh)
                return 0;
        }
        break;
    case 218:
        if (vbh || !vbl)
            return 0;
        if (vbl)
            return 1;
        break;
    case 217:
        if (vdh)
            return 0;
        if (!xsector[nXSector].at13_4 && !xsector[nXSector].at13_5 && floorZ-bottom > 0x2000)
            return 0;
        break;
    case 204:
    case 213:
    case 214:
    case 215:
    case 216:
    case 211:
    case 220:
    case 227:
    case 245:
        if (vdh)
            return 0;
        if (vdl || vbl)
            return 0;
        if (floorZ - bottom > 0x2000)
            return 0;
        break;
    case 203:
    case 210:
    default:
        if (vdh)
            return 0;
        if (!xsector[nXSector].at13_4 && !xsector[nXSector].at13_5 && floorZ - bottom > 0x2000)
            return 0;
        break;
    }
    return 1;
}

void aiChooseDirection(SPRITE *pSprite, XSPRITE *pXSprite, int a3)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 497);
    int vc = ((a3+1024-pSprite->ang)&2047)-1024;
    int nSin = Sin(pSprite->ang);
    int nCos = Cos(pSprite->ang);
    int t1 = dmulscale30(xvel[nSprite], nCos, yvel[nSprite], nSin);
    int t2 = dmulscale30(xvel[nSprite], nSin, -yvel[nSprite], nCos);
    int vsi = ((t1*15)>>12) / 2;
    int v8 = 341;
    if (vc < 0)
        v8 = -341;
    if (CanMove(pSprite, pXSprite->target, pSprite->ang+vc, vsi))
        pXSprite->at16_0 = pSprite->ang+vc;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang+vc/2, vsi))
        pXSprite->at16_0 = pSprite->ang+vc/2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang-vc/2, vsi))
        pXSprite->at16_0 = pSprite->ang-vc/2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang+v8, vsi))
        pXSprite->at16_0 = pSprite->ang+v8;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang, vsi))
        pXSprite->at16_0 = pSprite->ang;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang-v8, vsi))
        pXSprite->at16_0 = pSprite->ang-v8;
    else if (pSprite->flags&kSpriteFlag1)
        pXSprite->at16_0 = pSprite->ang+341;
    else // Weird..
        pXSprite->at16_0 = pSprite->ang+341;
    pXSprite->at17_3 = Chance(0x8000) ? 1 : -1;
    if (!CanMove(pSprite, pXSprite->target, pSprite->ang+pXSprite->at17_3*512, 512))
    {
        pXSprite->at17_3 = -pXSprite->at17_3;
        if (!CanMove(pSprite, pXSprite->target, pSprite->ang+pXSprite->at17_3*512, 512))
            pXSprite->at17_3 = 0;
    }
}

void aiMoveForward(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 588);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (klabs(nAng) > 341)
        return;
    xvel[nSprite] += mulscale30(pDudeInfo->at38, Cos(pSprite->ang));
    yvel[nSprite] += mulscale30(pDudeInfo->at38, Sin(pSprite->ang));
}

void aiMoveTurn(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 608);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nAng = ((pXSprite->at16_0+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->at44<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
}

void aiMoveDodge(SPRITE *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 621);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
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
}

void aiActivateDude(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 651);
    if (pXSprite->at1_6 == 0)
    {
        int ang = getangle(pXSprite->at20_0-pSprite->x, pXSprite->at24_0-pSprite->y);
        aiChooseDirection(pSprite, pXSprite, ang);
        pXSprite->at1_6 = 1;
    }
    switch (pSprite->type)
    {
    case 210:
    {
        DUDEEXTRA_GHOST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.ghost;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &ghostSearch);
        else
        {
            aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &ghostChase);
        }
        break;
    }
    case 201:
    case 202:
    case 247:
    case 248:
    case 249:
    {
        DUDEEXTRA_CULTIST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.cultist;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                {
                    if (pSprite->type == 201)
                        aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                    else
                        aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                }
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
            {
                if (pSprite->type == 201)
                    aiPlay3DSound(pSprite, 4003+Random(4), AI_SFX_PRIORITY_1, -1);
                else
                    aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            }
            switch (pXSprite->at17_6)
            {
            case 0:
                if (pSprite->type == 201)
                    aiNewState(pSprite, pXSprite, &fanaticChase);
                else
                    aiNewState(pSprite, pXSprite, &cultistChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case 230:
    {
        DUDEEXTRA_CULTIST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.cultist;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->type = 201;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                    aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistProneChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case 246:
    {
        DUDEEXTRA_CULTIST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.cultist;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->type = 202;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                    aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistProneChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case 240:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cultistBurnSearch);
        else
            aiNewState(pSprite, pXSprite, &cultistBurnChase);
        break;
    case 219:
    {
        DUDEEXTRA_BAT *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.bat;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pSprite->flags == 0)
            pSprite->flags = 9;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &batSearch);
        else
        {
            if (Chance(0xa000))
                aiPlay3DSound(pSprite, 2000, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &batChase);
        }
        break;
    }
    case 218:
    {
        DUDEEXTRA_EEL *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.eel;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &eelSearch);
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1501, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1500, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &eelChase);
        }
        break;
    }
    case 217:
    {
        DUDEEXTRA_GILLBEAST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.gillBeast;
        XSECTOR *pXSector = NULL;
        if (sector[pSprite->sectnum].extra > 0)
            pXSector = &xsector[sector[pSprite->sectnum].extra];
        pDudeExtraE->at0 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        if (pXSprite->target == -1)
        {
            if (pXSector && pXSector->at13_4)
                aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
            else
                aiNewState(pSprite, pXSprite, &gillBeastSearch);
        }
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1701, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1700, AI_SFX_PRIORITY_1, -1);
            if (pXSector && pXSector->at13_4)
                aiNewState(pSprite, pXSprite, &gillBeastSwimChase);
            else
                aiNewState(pSprite, pXSprite, &gillBeastChase);
        }
        break;
    }
    case 203:
    {
        DUDEEXTRA_ZOMBIEAXE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.zombieAxe;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieASearch);
        else
        {
            if (Chance(0xa000))
            {
                switch (Random(3))
                {
                case 1:
                    aiPlay3DSound(pSprite, 1104, AI_SFX_PRIORITY_1, -1);
                    break;
                case 2:
                    aiPlay3DSound(pSprite, 1105, AI_SFX_PRIORITY_1, -1);
                    break;
                case 3:
                    aiPlay3DSound(pSprite, 1103, AI_SFX_PRIORITY_1, -1);
                    break;
                default:
                    aiPlay3DSound(pSprite, 1103, AI_SFX_PRIORITY_1, -1);
                    break;
                }
            }
            aiNewState(pSprite, pXSprite, &zombieAChase);
        }
        break;
    }
    case 205:
    {
        DUDEEXTRA_ZOMBIEAXE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.zombieAxe;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->at34 == &zombieEIdle)
            aiNewState(pSprite, pXSprite, &zombieEUp);
        break;
    }
    case 244:
    {
        DUDEEXTRA_ZOMBIEAXE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.zombieAxe;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->at34 == &zombieSIdle)
            aiNewState(pSprite, pXSprite, &zombie13AC2C);
        break;
    }
    case 204:
    {
        DUDEEXTRA_ZOMBIEFAT *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.zombieFat;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1201, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1200, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &zombieFChase);
        }
        break;
    }
    case 241:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieABurnSearch);
        else
            aiNewState(pSprite, pXSprite, &zombieABurnChase);
        break;
    case 242:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
        else
            aiNewState(pSprite, pXSprite, &zombieFBurnChase);
        break;
    case 206:
    {
        DUDEEXTRA_GARGOYLE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.gargoyle;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
        }
        break;
    }
    case 207:
    {
        DUDEEXTRA_GARGOYLE *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.gargoyle;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1451, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
        }
        break;
    }
    case 208:
        if (Chance(0x4000))
            aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
        else
            aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
        aiNewState(pSprite, pXSprite, &gargoyleFMorph);
        break;
    case 209:
        if (Chance(0x4000))
            aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
        else
            aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
        aiNewState(pSprite, pXSprite, &gargoyleSMorph);
        break;
    case 227:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cerberusSearch);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &cerberusChase);
        }
        break;
    case 228:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cerberus2Search);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &cerberus2Chase);
        }
        break;
    case 211:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &houndSearch);
        else
        {
            aiPlay3DSound(pSprite, 1300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &houndChase);
        }
        break;
    case 212:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &handSearch);
        else
        {
            aiPlay3DSound(pSprite, 1900, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &handChase);
        }
        break;
    case 220:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &ratSearch);
        else
        {
            aiPlay3DSound(pSprite, 2100, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &ratChase);
        }
        break;
    case 245:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &innocentSearch);
        else
        {
            if (pXSprite->health > 0)
            {
                aiPlay3DSound(pSprite, 7000+Random(6), AI_SFX_PRIORITY_1, -1);
            }
            aiNewState(pSprite, pXSprite, &innocentChase);
        }
        break;
    case 229:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &tchernobogSearch);
        else
        {
            aiPlay3DSound(pSprite, 2350+Random(7), AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &tchernobogChase);
        }
        break;
    case 213:
    case 214:
    case 215:
        pSprite->flags |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1800, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &spidChase);
        }
        break;
    case 216:
    {
        DUDEEXTRA_SPIDER *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.spider;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->flags |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1853+Random(1), AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &spidChase);
        }
        break;
    }
    case 250:
    {
        DUDEEXTRA_TINYCALEB *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.tinyCaleb;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &tinycalebSearch);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &tinycalebSwimSearch);
                break;
            }
        }
        else
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &tinycalebChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &tinycalebSwimChase);
                break;
            }
        }
        break;
    }
    case 251:
    {
        DUDEEXTRA_BEAST *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.beast;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &beastSearch);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &beastSwimSearch);
                break;
            }
        }
        else
        {
            aiPlay3DSound(pSprite, 9009+Random(2), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->at17_6)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &beastChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &beastSwimChase);
                break;
            }
        }
        break;
    }
    case 221:
    case 223:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &podSearch);
        else
        {
            aiPlay3DSound(pSprite, pSprite->type == 223 ? 2453 : 2473, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &podChase);
        }
        break;
    case 222:
    case 224:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &tentacleSearch);
        else
        {
            aiPlay3DSound(pSprite, 2503, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &tentacleChase);
        }
        break;
    }
}

void aiSetTarget(XSPRITE *pXSprite, int x, int y, int z)
{
    pXSprite->target = -1;
    pXSprite->at20_0 = x;
    pXSprite->at24_0 = y;
    pXSprite->at28_0 = z;
}

void aiSetTarget(XSPRITE *pXSprite, int nTarget)
{
    dassert(nTarget >= 0 && nTarget < kMaxSprites, 1344);
    SPRITE *pTarget = &sprite[nTarget];
    if (pTarget->type >= kDudeBase && pTarget->type < kDudeMax)
    {
        if (nTarget != actSpriteOwnerToSpriteId(&sprite[pXSprite->reference]))
        {
            DUDEINFO *pDudeInfo = &dudeInfo[pTarget->type-kDudeBase];
            pXSprite->target = nTarget;
            pXSprite->at20_0 = pTarget->x;
            pXSprite->at24_0 = pTarget->y;
            pXSprite->at28_0 = pTarget->z-((pDudeInfo->atb*pTarget->yrepeat)<<2);
        }
    }
}


int aiDamageSprite(SPRITE *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDmgType, int nDamage)
{
    dassert(nSource < kMaxSprites, 1372);
    if (pXSprite->health == 0)
        return 0;
    pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);
    cumulDamage[pSprite->extra] += nDamage;
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nSprite = pXSprite->reference;
    if (nSource >= 0)
    {
        SPRITE *pSource = &sprite[nSource];
        if (pSource == pSprite)
            return 0;
        if (pXSprite->target == -1)
        {
            aiSetTarget(pXSprite, nSource);
            aiActivateDude(pSprite, pXSprite);
        }
        else if (nSource != pXSprite->target)
        {
            int t = nDamage;

            if (pSprite->type == pSource->type)
                t *= pDudeInfo->at2f;
            else
                t *= pDudeInfo->at2b;

            if (Chance(t))
            {
                aiSetTarget(pXSprite, nSource);
                aiActivateDude(pSprite, pXSprite);
            }
        }
        if (nDmgType == kDamageTesla)
        {
            DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
            pDudeExtra->at4 = 1;
        }
        switch (pSprite->type)
        {
        case 201:
        case 202:
        case 247:
        case 248:
            if (nDmgType != kDamageBurn)
            {
                if (!aiSeqPlaying(pSprite, 14) && pXSprite->at17_6 == 0)
                    aiNewState(pSprite, pXSprite, &cultistDodge);
                else if (aiSeqPlaying(pSprite, 14) && pXSprite->at17_6 == 0)
                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                else if (aiSeqPlaying(pSprite, 13) && (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2))
                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
            }
            else if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->at23 && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2))
            {
                pSprite->type = 240;
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[40].at2, dudeInfo[40].at2);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 245:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->at23 && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2))
            {
                pSprite->type = 239;
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[39].at2, dudeInfo[39].at2);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 240:
            if (Chance(0x4000) && gFrameClock > gDudeExtra[pSprite->extra].at0)
            {
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
            }
            if (Chance(0x600) && (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2))
            {
                pSprite->type = 201;
                pXSprite->at2c_0 = 0;
                aiNewState(pSprite, pXSprite, &cultistSwimGoto);
            }
            else if (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2)
            {
                pSprite->type = 202;
                pXSprite->at2c_0 = 0;
                aiNewState(pSprite, pXSprite, &cultistSwimGoto);
            }
            break;
        case 206:
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
            break;
        case 204:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->at23)
            {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
                pSprite->type = 242;
                aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
                actHealDude(pXSprite, dudeInfo[42].at2, dudeInfo[42].at2);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 250:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->at23 && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2))
            {
                pSprite->type = 239;
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[39].at2, dudeInfo[39].at2);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 249:
            if (pXSprite->health <= (unsigned int)pDudeInfo->at23)
            {
                pSprite->type = 251;
                aiPlay3DSound(pSprite, 9008, AI_SFX_PRIORITY_1, -1);
                aiNewState(pSprite, pXSprite, &beastMorphFromCultist);
                actHealDude(pXSprite, dudeInfo[51].at2, dudeInfo[51].at2);
            }
            break;
        case 203:
        case 205:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->at23)
            {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
                pSprite->type = 241;
                aiNewState(pSprite, pXSprite, &zombieABurnGoto);
                actHealDude(pXSprite, dudeInfo[41].at2, dudeInfo[41].at2);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        }
    }
    return nDamage;
}

void RecoilDude(SPRITE *pSprite, XSPRITE *pXSprite)
{
    BOOL v4 = Chance(0x8000);
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    if (pSprite->statnum == 6 && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
    {
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    switch (pSprite->type)
    {
    case 201:
    case 202:
    case 247:
    case 248:
    case 249:
        if (pSprite->type == 201)
            aiPlay3DSound(pSprite, 4013+Random(2), AI_SFX_PRIORITY_2, -1);
        else
            aiPlay3DSound(pSprite, 1013+Random(2), AI_SFX_PRIORITY_2, -1);
        if (!v4 && pXSprite->at17_6 == 0)
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &cultistRecoil);
        }
        else if (v4 && pXSprite->at17_6 == 0)
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
            else if (gGameOptions.nDifficulty > DIFFICULTY_0)
                aiNewState(pSprite, pXSprite, &cultistProneRecoil);
            else
                aiNewState(pSprite, pXSprite, &cultistRecoil);
        }
        else if (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2)
            aiNewState(pSprite, pXSprite, &cultistSwimRecoil);
        else
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &cultistRecoil);
        }
        break;
    case 240:
        aiNewState(pSprite, pXSprite, &cultistBurnGoto);
        break;
    case 204:
        aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4)
            aiNewState(pSprite, pXSprite, &zombieFTeslaRecoil);
        else
            aiNewState(pSprite, pXSprite, &zombieFRecoil);
        break;
    case 203:
    case 205:
        aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4 && pXSprite->at14_0 > pDudeInfo->at2/3)
            aiNewState(pSprite, pXSprite, &zombieATeslaRecoil);
        else if (pXSprite->at14_0 > pDudeInfo->at2/3)
            aiNewState(pSprite, pXSprite, &zombieARecoil2);
        else
            aiNewState(pSprite, pXSprite, &zombieARecoil);
        break;
    case 241:
        aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &zombieABurnGoto);
        break;
    case 242:
        aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
        break;
    case 206:
    case 207:
        aiPlay3DSound(pSprite, 1402, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &gargoyleFRecoil);
        break;
    case 227:
        aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4 && pXSprite->at14_0 > pDudeInfo->at2/3)
            aiNewState(pSprite, pXSprite, &cerberusTeslaRecoil);
        else
            aiNewState(pSprite, pXSprite, &cerberusRecoil);
        break;
    case 228:
        aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &cerberus2Recoil);
        break;
    case 211:
        aiPlay3DSound(pSprite, 1302, AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4)
            aiNewState(pSprite, pXSprite, &houndTeslaRecoil);
        else
            aiNewState(pSprite, pXSprite, &houndRecoil);
        break;
    case 229:
        aiPlay3DSound(pSprite, 2370+Random(2), AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &tchernobogRecoil);
        break;
    case 212:
        aiPlay3DSound(pSprite, 1902, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &handRecoil);
        break;
    case 220:
        aiPlay3DSound(pSprite, 2102, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &ratRecoil);
        break;
    case 219:
        aiPlay3DSound(pSprite, 2002, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &batRecoil);
        break;
    case 218:
        aiPlay3DSound(pSprite, 1502, AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &eelRecoil);
        break;
    case 217:
    {
        XSECTOR *pXSector = NULL;
        if (sector[pSprite->sectnum].extra > 0)
            pXSector = &xsector[sector[pSprite->sectnum].extra];
        aiPlay3DSound(pSprite, 1702, AI_SFX_PRIORITY_2, -1);
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastSwimRecoil);
        else
            aiNewState(pSprite, pXSprite, &gillBeastRecoil);
        break;
    }
    case 210:
        aiPlay3DSound(pSprite, 1602, AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4)
            aiNewState(pSprite, pXSprite, &ghostTeslaRecoil);
        else
            aiNewState(pSprite, pXSprite, &ghostRecoil);
        break;
    case 213:
    case 214:
    case 215:
        aiPlay3DSound(pSprite, 1802+Random(1), AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &spidDodge);
        break;
    case 216:
        aiPlay3DSound(pSprite, 1851+Random(1), AI_SFX_PRIORITY_2, -1);
        aiNewState(pSprite, pXSprite, &spidDodge);
        break;
    case 245:
        aiPlay3DSound(pSprite, 7007+Random(2), AI_SFX_PRIORITY_2, -1);
        if (pDudeExtra->at4)
            aiNewState(pSprite, pXSprite, &innocentTeslaRecoil);
        else
            aiNewState(pSprite, pXSprite, &innocentRecoil);
        break;
    case 250:
        if (pXSprite->at17_6 == 0)
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &tinycalebTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &tinycalebRecoil);
        }
        else if (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2)
            aiNewState(pSprite, pXSprite, &tinycalebSwimRecoil);
        else
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &tinycalebTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &tinycalebRecoil);
        }
        break;
    case 251:
        aiPlay3DSound(pSprite, 9004+Random(2), AI_SFX_PRIORITY_2, -1);
        if (pXSprite->at17_6 == 0)
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &beastTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &beastRecoil);
        }
        else if (pXSprite->at17_6 == 1 || pXSprite->at17_6 == 2)
            aiNewState(pSprite, pXSprite, &beastSwimRecoil);
        else
        {
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &beastTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &beastRecoil);
        }
        break;
    case 221:
    case 223:
        aiNewState(pSprite, pXSprite, &podRecoil);
        break;
    case 222:
    case 224:
        aiNewState(pSprite, pXSprite, &tentacleRecoil);
        break;
    default:
        aiNewState(pSprite, pXSprite, &genRecoil);
        break;
    }
    pDudeExtra->at4 = 0;
    }
}

void aiThinkTarget(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 1942);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    if (!Chance(pDudeInfo->at33))
        return;
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        PLAYER *pPlayer = &gPlayer[p];
        if (pSprite->owner == pPlayer->at5b || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, 13) > 0)
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

void func_5F15C(SPRITE *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 2006);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    if (!Chance(pDudeInfo->at33))
        return;
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        PLAYER *pPlayer = &gPlayer[p];
        if (pSprite->owner == pPlayer->at5b || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, 13) > 0)
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
            if (cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z- height, pSprite->sectnum))
            {
                int nAngle = getangle(dx, dy);
                int nDeltaAngle = ((1024+nAngle-pSprite->ang)&2047)-1024;
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
    if (pXSprite->at1_6)
    {
        byte va4[(kMaxSectors+7)>>3];
        int x = pSprite->x;
        int y = pSprite->z;
        int nSector = pSprite->sectnum;
        gAffectedSectors[0] = -1;
        gAffectedXWalls[0] = -1;
        GetClosestSpriteSectors(nSector, x, y, 400, gAffectedSectors, va4, gAffectedXWalls);
        for (int nSprite2 = headspritestat[6]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
        {
            SPRITE* pSprite2 = &sprite[nSprite2];
            int x = pSprite2->x;
            int y = pSprite2->y;
            int dx = x - pSprite->x;
            int dy = y - pSprite->y;
            int nDist = approxDist(dx, dy);
            if (pSprite2->type == 245)
            {
                DUDEINFO* pDudeInfo = &dudeInfo[pSprite2->type - kDudeBase];
                if (nDist <= pDudeInfo->at17 || nDist <= pDudeInfo->at13)
                {
                    int nAngle = getangle(dx, dy);
                    int nDeltaAngle = ((1024+nAngle-pSprite->ang)&2047)-1024;
                    aiSetTarget(pXSprite, pSprite2->index);
                    aiActivateDude(pSprite, pXSprite);
                    return;
                }
            }
        }
    }
}

void aiProcessDudes(void)
{
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        SPRITE *pSprite = &sprite[nSprite];
        if (pSprite->flags&kSpriteFlag5)
            continue;
        int nXSprite = pSprite->extra;
        XSPRITE *pXSprite = &xsprite[nXSprite];
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
        if (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8)
            continue;
        if (pXSprite->health == 0)
            continue;
        pXSprite->at32_0 = ClipLow(pXSprite->at32_0-4, 0);
        if (pXSprite->at34->at10)
            pXSprite->at34->at10(pSprite, pXSprite);
        if (pXSprite->at34->at14 && (gFrame&3) == (nSprite&3))
            pXSprite->at34->at14(pSprite, pXSprite);
        if (pXSprite->at32_0 == 0 && pXSprite->at34->at18)
        {
            if (pXSprite->at34->at8 > 0)
                aiNewState(pSprite, pXSprite, pXSprite->at34->at18);
            else if (seqGetStatus(3, nXSprite) < 0)
                aiNewState(pSprite, pXSprite, pXSprite->at34->at18);
        }
        if (pXSprite->health > 0 && ((pDudeInfo->at27<<4) <= cumulDamage[nXSprite]))
        {
            pXSprite->at14_0 = cumulDamage[nXSprite];
            RecoilDude(pSprite, pXSprite);
        }
    }
    memset(cumulDamage, 0, sizeof(cumulDamage));
}

void aiInit(void)
{
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        aiInitSprite(&sprite[nSprite]);
    }
}

void aiInitSprite(SPRITE *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSector = pSprite->sectnum;
    XSECTOR *pXSector = NULL;
    if (sector[nSector].extra > 0)
        pXSector = &xsector[sector[nSector].extra];
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    pDudeExtra->at4 = 0;
    pDudeExtra->at0 = 0;
    switch (pSprite->type)
    {
    case 201:
    case 202:
    case 247:
    case 248:
    case 249:
    {
        DUDEEXTRA_CULTIST *pDudeExtraE = &gDudeExtra[nXSprite].at6.cultist;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cultistIdle);
        break;
    }
    case 230:
    {
        DUDEEXTRA_CULTIST* pDudeExtraE = &pDudeExtra->at6.cultist;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &fanaticProneIdle);
        break;
    }
    case 246:
    {
        DUDEEXTRA_CULTIST* pDudeExtraE = &pDudeExtra->at6.cultist;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cultistProneIdle);
        break;
    }
    case 204:
    {
        DUDEEXTRA_ZOMBIEFAT* pDudeExtraE = &pDudeExtra->at6.zombieFat;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieFIdle);
        break;
    }
    case 203:
    {
        DUDEEXTRA_ZOMBIEAXE* pDudeExtraE = &pDudeExtra->at6.zombieAxe;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieAIdle);
        break;
    }
    case 244:
    {
        DUDEEXTRA_ZOMBIEAXE* pDudeExtraE = &pDudeExtra->at6.zombieAxe;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieSIdle);
        pSprite->flags &= ~1;
        break;
    }
    case 205:
    {
        DUDEEXTRA_ZOMBIEAXE* pDudeExtraE = &pDudeExtra->at6.zombieAxe;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieEIdle);
        pSprite->flags &= ~1;
        break;
    }
    case 206:
    case 207:
    {
        DUDEEXTRA_GARGOYLE* pDudeExtraE = &pDudeExtra->at6.gargoyle;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &gargoyleFIdle);
        break;
    }
    case 208:
    case 209:
        aiNewState(pSprite, pXSprite, &gargoyleStatueIdle);
        break;
    case 227:
    {
        DUDEEXTRA_CERBERUS* pDudeExtraE = &pDudeExtra->at6.cerberus;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cerberusIdle);
        break;
    }
    case 211:
        aiNewState(pSprite, pXSprite, &houndIdle);
        break;
    case 212:
        aiNewState(pSprite, pXSprite, &handIdle);
        break;
    case 210:
    {
        DUDEEXTRA_GHOST* pDudeExtraE = &pDudeExtra->at6.ghost;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &ghostIdle);
        break;
    }
    case 245:
        aiNewState(pSprite, pXSprite, &innocentIdle);
        break;
    case 220:
        aiNewState(pSprite, pXSprite, &ratIdle);
        break;
    case 218:
    {
        DUDEEXTRA_EEL* pDudeExtraE = &pDudeExtra->at6.eel;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &eelIdle);
        break;
    }
    case 217:
        if (pXSector && pXSector->at13_4)
            aiNewState(pSprite, pXSprite, &gillBeastIdle);
        else
            aiNewState(pSprite, pXSprite, &gillBeastIdle);
        break;
    case 219:
    {
        DUDEEXTRA_BAT* pDudeExtraE = &pDudeExtra->at6.bat;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &batIdle);
        break;
    }
    case 213:
    case 214:
    case 215:
    {
        DUDEEXTRA_SPIDER* pDudeExtraE = &gDudeExtra[nXSprite].at6.spider;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &spidIdle);
        break;
    }
    case 216:
    {
        DUDEEXTRA_SPIDER* pDudeExtraE = &pDudeExtra->at6.spider;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &spidIdle);
        break;
    }
    case 229:
    {
        DUDEEXTRA_TCHERNOBOG* pDudeExtraE = &pDudeExtra->at6.tchernobog;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &tchernobogIdle);
        break;
    }
    case 250:
        aiNewState(pSprite, pXSprite, &tinycalebIdle);
        break;
    case 251:
        aiNewState(pSprite, pXSprite, &beastIdle);
        break;
    case 221:
    case 223:
        aiNewState(pSprite, pXSprite, &podIdle);
        break;
    case 222:
    case 224:
        aiNewState(pSprite, pXSprite, &tentacleIdle);
        break;
    default:
        aiNewState(pSprite, pXSprite, &genIdle);
        break;
    }
    aiSetTarget(pXSprite, 0, 0, 0);
    pXSprite->at32_0 = 0;
    switch (pSprite->type)
    {
    case 213:
    case 214:
    case 215:
        if (pSprite->cstat&kSpriteStat3)
            pSprite->flags |= 9;
        else
            pSprite->flags |= 15;
        break;
    case 206:
    case 207:
    case 210:
    case 218:
    case 219:
        pSprite->flags |= 9;
        break;
    case 217:
        if (pXSector && pXSector->at13_4)
            pSprite->flags |= 9;
        else
            pSprite->flags |= 15;
        break;
    case 205:
    case 244:
        pSprite->flags = 7;
        break;
    default:
        pSprite->flags = 15;
        break;
    }
}

class AILoadSave : public LoadSave
{
    virtual void Load(void);
    virtual void Save(void);
};

void AILoadSave::Load(void)
{
    Read(cumulDamage, sizeof(cumulDamage));
    Read(gDudeSlope, sizeof(gDudeSlope));
}

void AILoadSave::Save(void)
{
    Write(cumulDamage, sizeof(cumulDamage));
    Write(gDudeSlope, sizeof(gDudeSlope));
}

static AILoadSave myLoadSave;
