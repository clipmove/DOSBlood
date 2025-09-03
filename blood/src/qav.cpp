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
#include "typedefs.h"
#include "build.h"
#include "debug4g.h"
#include "globals.h"
#include "misc.h"
#include "qav.h"
#include "tile.h"


#define kMaxClients 64
static void (*clientCallback[kMaxClients])(int, void *);
static int nClients;

struct TILE_FRAME_OLD {
    int x;
    int y;
    byte bLerp;
};
static TILE_FRAME_OLD oldFrame[8];

static FRAMEINFO *pInterpCurFrame = NULL;
static TILE_FRAME fInterpOldTiles[8];
static int nInterpLastClock = 0;
static int nInterpLastFract = 0;


int qavRegisterClient(void(*pClient)(int, void *))
{
    dassert(nClients < kMaxClients, 31);
    int id = nClients++;
    clientCallback[id] = pClient;

    return id;
}


static void DrawFrame(int x, int y, TILE_FRAME *pTile, int stat, int shade, int palnum, byte bUseQ16, TILE_FRAME_OLD *pTileInterp, long nInterpolate)
{
    int angle = pTile->angle;
    byte pal;
    stat |= pTile->stat;
    if (stat & 0x100)
    {
        angle = (angle+1024)&2047;
        stat &= ~0x100;
        stat ^= 0x4;
    }
    pal = palnum > 0 ? (char)palnum : (char)pTile->palnum;
    int tileX = pTile->x;
    int tileY = pTile->y;
    int tileZ = pTile->z;
    tileX <<= 16;
    tileY <<= 16;
    if (!bUseQ16)
    {
        x <<= 16;
        y <<= 16;
    }
    if (pTileInterp && pTileInterp->bLerp)
    {
        if ((klabs(pTileInterp->x - tileX) < 32<<16) && (klabs(pTileInterp->y - tileY) < 32<<16))
        {
            tileX = interpolate16(pTileInterp->x, tileX, nInterpolate);
            tileY = interpolate16(pTileInterp->y, tileY, nInterpolate);
        }
    }
    x += tileX;
    y += tileY;
    rotatesprite(x, y, tileZ, angle, 
                 pTile->picnum, ClipRange(pTile->shade + shade, -128, 127), pal, stat,
                 windowx1, windowy1, windowx2, windowy2);
}

static byte PrepareInterpolate(FRAMEINFO *pFrame, long *pTicks, int nFrames)
{
    if (nFrames <= 1) // single frame QAV, cannot interpolate
    {
        pInterpCurFrame = NULL;
        return 0;
    }
    if (pInterpCurFrame == pFrame) // don't bother updating to last frame, we're still on the same frame
    {
        const int nTableFracts[4] = {0x0000, 0x4000, 0x8000, 0xC000}; // 0, 0.25, 0.5, 0.75
        if (nInterpLastClock == gGameClock) // we're still within the same quarter tick, do not bother fetching new nInterpLastFract
        {
            *pTicks = nInterpLastFract;
            return 1;
        }
        nInterpLastFract = gGameClock - nInterpLastClock;
        if (nInterpLastFract >= 4 || nInterpLastFract < 0) // we've gone below/beyond a quarter tick, do not even attempt to interpolat
            return 0;
        nInterpLastFract = nTableFracts[nInterpLastFract];
        *pTicks = nInterpLastFract;
        return 1;
    }
    *pTicks = 0; // new frame, set to 0
    nInterpLastClock = gGameClock; // update on new frame
    oldFrame[0].bLerp = oldFrame[1].bLerp = oldFrame[2].bLerp = oldFrame[3].bLerp = oldFrame[4].bLerp = oldFrame[5].bLerp = oldFrame[6].bLerp = oldFrame[7].bLerp = 0;
    if (pInterpCurFrame != NULL) // only save the latest QAV frames if we've played a new QAV
    {
        for (int i = 0; i < 8; i++)
        {
            const int nTileNew = pFrame->tiles[i].picnum;
            if (nTileNew <= 0) // skip this layer
                continue;
            if (tilesizx[nTileNew] < 4 && tilesizy[nTileNew] < 4) // tile is too small, don't attempt to lerp
                continue;
            for (int j = 0; j < 8; j++) // search for the most likely layer that matches the layer of the new frame
            {
                const int nTileOld = fInterpOldTiles[j].picnum;
                if (nTileOld != nTileNew)
                {
                    if (klabs(nTileOld - nTileNew) > 10) // tile IDs are too far apart, skip
                        continue;
                    if (tilesizx[nTileOld] != tilesizx[nTileNew] || tilesizy[nTileOld] != tilesizy[nTileNew]) // if the tile and tile sizes are different, skip
                        continue;
                }
                if (klabs(fInterpOldTiles[j].x - pFrame->tiles[i].x) > 24) // the step is too big, and will result in a bad frame, likely not the tile we're trying to assign
                    continue;
                if (klabs(fInterpOldTiles[j].y - pFrame->tiles[i].y) > 24)
                    continue;
                oldFrame[i].x = fInterpOldTiles[j].x<<16;
                oldFrame[i].y = fInterpOldTiles[j].y<<16;
                oldFrame[i].bLerp = 1;
            }
        }
    }
    else // our last frame pointer is null, cache from current frame
    {
        for (int i = 0; i < 8; i++)
        {
            const int nTileNew = pFrame->tiles[i].picnum;
            if (nTileNew <= 0) // skip this layer
                continue;
            oldFrame[i].x = pFrame->tiles[i].x<<16;
            oldFrame[i].y = pFrame->tiles[i].y<<16;
            oldFrame[i].bLerp = 1;
        }
    }
    pInterpCurFrame = pFrame;
    memcpy(fInterpOldTiles, pFrame->tiles, sizeof(fInterpOldTiles)); // copy entire old frame for interp checking next frame
    return 1;
}

void QAV::Draw(long ticks, int stat, int shade, int palnum, byte bUseQ16, byte bInterpolate)
{
    dassert(ticksPerFrame > 0, 72);
    int nFrame = ticks / ticksPerFrame;
    dassert(nFrame >= 0 && nFrame < nFrames, 74);
    FRAMEINFO *pFrame = &frames[nFrame];
    if (bInterpolate) // cache old frame for interpolate logic
        bInterpolate = PrepareInterpolate(pFrame, &ticks, nFrames);
    for (int i = 0; i < 8; i++)
    {
        if (pFrame->tiles[i].picnum > 0)
            DrawFrame(x, y, &pFrame->tiles[i], stat, shade, palnum, bUseQ16, bInterpolate ? &oldFrame[i] : NULL, ticks);
    }
}

void QAV::Play(long start, long end, int nCallback, void *pData)
{
    dassert(ticksPerFrame > 0, 87);
    int frame;
    int ticks;
    if (start < 0)
        frame = (start + 1) / ticksPerFrame;
    else
        frame = start / ticksPerFrame + 1;
    
    for (ticks = ticksPerFrame * frame; ticks <= end; frame++, ticks += ticksPerFrame)
    {
        if (frame >= 0 && frame < nFrames)
        {
            FRAMEINFO *pFrame = &frames[frame];
            SOUNDINFO *pSound = &pFrame->sound;
            if (pSound->sound > 0)
            {
                if (!pSprite)
                    PlaySound(pSound->sound);
                else
                    PlaySound3D(pSprite, pSound->sound, 16+pSound->at4, 6);
            }
            if (pFrame->nCallbackId > 0 && nCallback != -1)
            {
                clientCallback[nCallback](pFrame->nCallbackId, pData);
            }
        }
    }
}

void QAV::Preload(void)
{
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (frames[i].tiles[j].picnum >= 0)
                tilePreloadTile(frames[i].tiles[j].picnum);
        }
    }
}
