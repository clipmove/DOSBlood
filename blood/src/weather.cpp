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
#include "trig.h"
#include "misc.h"

#include "actor.h"
#include "gameutil.h"
#include "levels.h"
#include "weather.h"

CWeather gWeather;

CWeather::CWeather()
{
    nDraw.b = 0;
    nWidth = 0;
    nHeight = 0;
    nOffsetX = 0;
    nOffsetY = 0;
    nScaleFactor = 0;
    memset(YLookup, 0, sizeof(YLookup));
    nCount = 0;
    nLimit = kMaxVectors;
    nGravity = 0;
    nWindX = 0;
    nWindY = 0;
    nWindXOffset = 0;
    nWindYOffset = 0;
    memset(nPos, 0, sizeof(nPos));
    nPalColor = 1;
    nPalShift = 0;
    nScaleTableWidth = 0;
    nScaleTableFov = 0;
    nFovV = 0;
    memset(nScaleTable, 0, sizeof(nScaleTable));
    memset(nColorTable, 0, sizeof(nColorTable));
    nLastFrameClock = 0;
    nWeatherCheat = WEATHERTYPE_NONE;
    nWeatherCur = WEATHERTYPE_NONE;
    nWeatherForecast = WEATHERTYPE_NONE;
    nWeatherOverride = 0;
    nWeatherOverrideType = WEATHERTYPE_NONE;
    nWeatherOverrideTypeInside = WEATHERTYPE_NONE;
    nWeatherOverrideWindX = 0;
    nWeatherOverrideWindY = 0;
    nWeatherOverrideGravity = 0;
}

CWeather::~CWeather()
{
    nDraw.b = 0;
    nWidth = 0;
    nHeight = 0;
    nOffsetX = 0;
    nOffsetY = 0;
    nScaleFactor = 0;
    memset(YLookup, 0, sizeof(YLookup));
    nCount = 0;
    nLimit = kMaxVectors;
    nGravity = 0;
    nWindX = 0;
    nWindY = 0;
    nWindXOffset = 0;
    nWindYOffset = 0;
    memset(nPos, 0, sizeof(nPos));
    nPalColor = 1;
    nPalShift = 0;
    nScaleTableWidth = 0;
    nScaleTableFov = 0;
    nFovV = 0;
    memset(nScaleTable, 0, sizeof(nScaleTable));
    memset(nColorTable, 0, sizeof(nColorTable));
    nLastFrameClock = 0;
    nWeatherCheat = WEATHERTYPE_NONE;
    nWeatherCur = WEATHERTYPE_NONE;
    nWeatherForecast = WEATHERTYPE_NONE;
    nWeatherOverride = 0;
    nWeatherOverrideType = WEATHERTYPE_NONE;
    nWeatherOverrideTypeInside = WEATHERTYPE_NONE;
    nWeatherOverrideWindX = 0;
    nWeatherOverrideWindY = 0;
    nWeatherOverrideGravity = 0;
}

void CWeather::RandomizeVectors(void)
{
    for (int i = 0; i < kMaxVectors; i++)
    {
        nPos[i][0] = krand()&0x3fff;
        nPos[i][1] = krand()&0x3fff;
        nPos[i][2] = krand()&0x3fff;
    }
    nWindXOffset = krand()&0x3fff;
    nWindYOffset = krand()&0x3fff;
}

void CWeather::SetViewport(int nX, int nY, int nXOffset0, int nXOffset1, int nYOffset0, int nYOffset1, int nFov)
{
    nWidth = (nXOffset1-nXOffset0)+1;
    nHeight = (nYOffset1-nYOffset0)+1;
    nOffsetX = nXOffset0;
    nOffsetY = nYOffset0;
    if (nX < 320 || nY < 200 || nOffsetX < 0 || nOffsetY < 0 || nWidth + nOffsetX > nX || nHeight + nOffsetY > nY) // something went very wrong, disable weather effects
    {
        SetWeatherType(WEATHERTYPE_NONE, 0);
        return;
    }
    nScaleFactor = divscale16(nHeight<<16, 200<<16); // scale to viewport
    for (int i = 0; i < nY; i++)
        YLookup[i] = nX * i;
    if (nScaleTableFov != nFov)
    {
        nFovV = nFov;
    }
    if (nScaleTableFov != nFov || nScaleTableWidth != nWidth)
    {
        nScaleTable[0] = 0; // index 0 is unused (depth 0 is invalid/culled earlier) - set to 0 to be safe
        const int nNumerator = nWidth<<15;
        for (int i = 1; i < kScaleTableSize; i++)
        {
            const int nScale = divscale16(nNumerator, i<<16);
            nScaleTable[i] = divscale16(nScale, nFovV);
        }
    }
    nScaleTableWidth = nWidth;
    nScaleTableFov = nFov;
}

void CWeather::SetParticles(short nCount, short nLimit)
{
    dassert(nCount >= 0 && nCount <= kMaxVectors, __LINE__);
    if (nLimit < 0 || nLimit > kMaxVectors)
        nLimit = kMaxVectors;
    this->nCount = nCount;
    this->nLimit = nLimit;
    if (!nCount)
        nWeatherCur = nWeatherForecast = WEATHERTYPE_NONE;
}

void CWeather::SetGravity(short nGravity, char bVariance)
{
    this->nGravity = nGravity;
    this->nDraw.bGravityVariance = bVariance & 1;
}

void CWeather::SetWind(short nX, short nY)
{
    nWindX = nX;
    nWindY = nY;
}

void CWeather::RandomWind(char bHeavyWind, unsigned int uMapCRC)
{
    nWindX = !bHeavyWind ? (uMapCRC&0x3f) - 0x20 : (uMapCRC&0x7f) - 0x40;
    uMapCRC >>= 16;
    nWindY = !bHeavyWind ? (uMapCRC&0x3f) - 0x20 : (uMapCRC&0x7f) - 0x40;
}

void CWeather::SetTranslucency(int a1)
{
    nDraw.nTransparent = ClipRange(a1, 0, 2);
}

void CWeather::SetColor(unsigned char a1)
{
    nPalColor = ClipRange(a1, 1, 255); // color picked is always minus 1, so don't allow anything less than 1
    UpdateColorTable();
}

void CWeather::SetColorShift(char a1)
{
    nPalShift = ClipRange(a1, 0, 2);
    UpdateColorTable();
}

void CWeather::UpdateColorTable(void)
{
    const int kDepthStep = 8192 / kColorTableSize; // potential range for nDepth is 5-8191, or 0-31 after shifting
    for (int i = 0, nDepth = 0; i < kColorTableSize; i++, nDepth += kDepthStep)
    {
        const int nDepthColorShift = ClipLow(nDepth >> (nPalShift + 8), 1); // max range is 1-31
        const byte nColor = ClipRange(nPalColor - nDepthColorShift, 0, 255); // clamp color
        nColorTable[i] = nColor;
    }
}

void CWeather::SetShape(char a1)
{
    nDraw.bShape = a1&3;
}

void CWeather::SetStaticView(char a1)
{
    nDraw.bStaticView = a1;
}

void CWeather::Initialize(int nCount)
{
    nDraw.bActive = 1;
    nDraw.bShape = 1;
    nScaleTableWidth = 0;
    nScaleTableFov = 0;
    nFovV = 0;
    nLastFrameClock = 0;
    nWeatherCur = WEATHERTYPE_NONE;
    nWeatherForecast = WEATHERTYPE_NONE;
    nWeatherOverride = 0;
    RandomizeVectors();
    SetParticles(nCount, kMaxVectors);
}

void CWeather::Draw(char *pBuffer, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int *pYLookup, long nX, long nY, long nZ, int nAng, int nPitch, int nHoriz, int nCount, int nDelta)
{
    dassert(pBuffer != NULL, __LINE__);
    dassert(pYLookup != NULL, __LINE__);
    dassert(nCount > 0 && nCount <= kMaxVectors, __LINE__);

    // move to first pixel within framebuffer
    pBuffer += pYLookup[nOffsetY] + nOffsetX;

    // adjust to starfield relative scale
    if (!nDraw.bStaticView)
    {
        nX <<= 1;
        nY <<= 1;
        nZ >>= 3;
    }
    else
    {
        nX = 0;
        nY = 0;
        nZ = 0;
    }

    // calculate wind offsets
    if (nWindX || nWindY)
    {
        nWindXOffset += mulscale16(nWindX, nDelta);
        nWindYOffset += mulscale16(nWindY, nDelta);
    }
    nX += nWindXOffset;
    nY += nWindYOffset;

    // scale pitch and horizon to buffer resolution (default res is 320x200)
    nPitch = mulscale16(nPitch<<16, nScaleFactor);
    nPitch = divscale16(nPitch, nFovV);
    nHoriz = mulscale16(nHoriz<<16, nScaleFactor);
    nPitch += nHoriz;
    nPitch >>= 16;

    const int nCos = Cos(nAng)>>16;
    const int nSin = Sin(nAng)>>16;
    const int bShape = nDraw.bShape;
    const int bTransparent = nDraw.nTransparent;
    const int nMaxPixelSize = nWidth>>7; // use screen width to control max pixel size
    const int nGrav = mulscale16(nGravity, nDelta);
    const int nGravityFast = nDraw.bGravityVariance && (nGrav != 0) ? nGrav - (nGrav >> 2) : nGrav;

    for (int i = 0; i < nCount; i++)
    {
        // calculate and wrap X/Y
        const int relX = ((nPos[i][1] - nX) & 0x3fff) - 0x2000;
        const int relY = ((nPos[i][0] - nY) & 0x3fff) - 0x2000;

        // depth in rotated/view space
        const int nDepth = (relX * nCos + relY * nSin)>>14;
        if (nDepth <= 4)
            continue;

        // cull by radius in rotated space
        const int nLatOffset = (relY * nCos - relX * nSin)>>14;
        if (nDepth * nDepth + nLatOffset * nLatOffset >= 0x4000000) // < 8192*8192
            continue;

        // perspective scale with fov adjustment (uses precomputed table instead of divscale16)
        const int nScale = nScaleTable[nDepth]; // potential range for nDepth is 5-8191
        const unsigned int screenX = ((nLatOffset * nScale)>>16) + (nWidth>>1);
        nPos[i][2] += i&4 ? nGravityFast : nGrav;
        if (screenX < (unsigned)nWidth) // if within screen bounds
        {
            // wrapping/centering logic for Z
            const int relZ = ((nPos[i][2] - nZ)&0x3fff) - 0x2000;
            unsigned int screenY = nPitch + ((nScale * relZ)>>16);
            if (screenY < (unsigned)nHeight) // if within screen bounds
            {
                // size/palette color calculation
                const int nSize = ClipHigh(nScale>>12, nMaxPixelSize); // why did I pick 12? because it looked the best
                const byte nColor = nColorTable[nDepth>>8]; // potential range for nDepth is 5-8191, or 0-31 after shift

                if (nSize <= 1) // if size is a pixel, don't bother calculating box fill
                {
                    for (int j = 1<<bShape; j > 0 && screenY > 0; j--, screenY--)
                    {
                        char *pDest = pBuffer + pYLookup[screenY] + screenX;
                        switch (bTransparent)
                        {
                        case 0:
                            *pDest = nColor;
                            break;
                        case 1:
                            *pDest = transluc[(nColor<<8) + *pDest];
                            break;
                        case 2:
                            *pDest = transluc[(*pDest<<8) + nColor];
                            break;
                        }
                    }
                    continue;
                }
                // do block fill
                const int cx = (int)screenX;
                const int cy = (int)screenY;
                const int nHalfX = nSize >> 1;
                const int nHalfY = !bShape ? nSize>>1 : nSize * bShape;
                switch (bTransparent)
                {
                case 0: // opaque path: fill size x size
                {
                    int yStart = cy - nHalfY;
                    int yEnd   = cy + nHalfY;
                    if (yStart < 0) yStart = 0;
                    if (yEnd >= nHeight) yEnd = nHeight - 1;

                    int xStart = cx - nHalfX;
                    int xEnd   = cx + nHalfX;
                    if (xStart < 0) xStart = 0;
                    if (xEnd >= nWidth) xEnd = nWidth - 1;

                    if (yStart <= yEnd && xStart <= xEnd)
                    {
                        size_t len = (size_t)(xEnd - xStart + 1);
                        for (int yy = yStart; yy <= yEnd; yy++)
                        {
                            char *pDest = pBuffer + pYLookup[yy];
                            memset(pDest + xStart, (int)nColor, len);
                        }
                    }
                    break;
                }
                case 1: // translucency/blend path: blend per-pixel using transluc table
                {
                    for (int yy = cy - nHalfY; yy <= cy + nHalfY; yy++)
                    {
                        if ((unsigned)yy >= (unsigned)nHeight) continue;
                        char *pDest = pBuffer + pYLookup[yy];
                        for (int xx = cx - nHalfX; xx <= cx + nHalfX; xx++)
                        {
                            if ((unsigned)xx >= (unsigned)nWidth) continue;
                            byte dst = (byte)pDest[xx];
                            pDest[xx] = transluc[(nColor<<8) + dst];
                        }
                    }
                    break;
                }
                case 2: // translucency/blend path: blend per-pixel using transluc table
                {
                    for (int yy = cy - nHalfY; yy <= cy + nHalfY; yy++)
                    {
                        if ((unsigned)yy >= (unsigned)nHeight) continue;
                        char *pDest = pBuffer + pYLookup[yy];
                        for (int xx = cx - nHalfX; xx <= cx + nHalfX; xx++)
                        {
                            if ((unsigned)xx >= (unsigned)nWidth) continue;
                            byte dst = (byte)pDest[xx];
                            pDest[xx] = transluc[(dst<<8) + nColor];
                        }
                    }
                    break;
                }
                }
            }
        }
    }
}

void CWeather::Draw(long nX, long nY, long nZ, int nAng, int nPitch, int nHoriz, int nCount, long nClock, int nInterpolate, unsigned int uMapCRC)
{
    const char bActive = Status();
    if (!bActive)
        return;
    nClock += mulscale16(1, nInterpolate<<2);
    int nDelta = (nClock - nLastFrameClock)<<16;
    nLastFrameClock = nClock;
    if (nCount == -1)
        nCount = this->nCount;
    if (bActive && nCount > 0)
        Draw((char*)frameplace, nWidth, nHeight, nOffsetX, nOffsetY, YLookup, nX, nY, nZ, nAng, nPitch, nHoriz, nCount, nDelta);
    nDelta >>= 16;
    if (nWeatherForecast == nWeatherCur) // increase until reached weather limit
    {
        nCount += nDelta * 16;
        SetCount(nCount);
    }
    else if (nCount && (nWeatherForecast != nWeatherCur)) // changed weather type, fade out then switch to new weather type
    {
        nCount -= nDelta * 56;
        SetCount(nCount);
    }
    else if (!nCount && (nWeatherForecast != WEATHERTYPE_NONE)) // fade out complete, now switch to new weather
    {
        nWindXOffset = krand()&0x3fff;
        nWindYOffset = krand()&0x3fff;
        SetWeatherType(nWeatherForecast, uMapCRC);
    }
}

void CWeather::LoadPreset(unsigned int uMapCRC)
{
    nWeatherCheat = WEATHERTYPE_NONE;
    switch (uMapCRC)
    {
    case 0xBBF1A5D5: // e1m3
    case 0xF524ACA4: // e5m2
    case 0xFE99F0E7: // e5m6
        SetWeatherOverride(WEATHERTYPE_SNOW, WEATHERTYPE_DUST, 0, -96, 32);
        break;
    case 0xAEC06508: // e1m5
    case 0xFA1A3218: // e4m1
    case 0x2D6A6F3D: // e4m3
    case 0x98FDBE0E: // e6m4
        SetWeatherOverride(WEATHERTYPE_RAINHARD, WEATHERTYPE_DUST, 32, 0, 96);
        break;
    case 0xCA80EAA3: // e2m1
    case 0x29D27D07: // e2m2
    case 0xE6B88CA6: // e2m3
    case 0x6AF2A719: // e2m4
    case 0xA0639DE5: // e4m6
        SetWeatherOverride(WEATHERTYPE_SNOW, WEATHERTYPE_DUST, 0, 4, 24);
        break;
    case 0xBA5DB227: // e1m2
    case 0xD64D2666: // e2m5
    case 0x09E3434D: // e2m6
    case 0x0FFF85AC: // e2m7
    case 0x602296E1: // e2m8
    case 0xF369A447: // e2m9
        SetWeatherOverride(WEATHERTYPE_SNOWHARD, WEATHERTYPE_DUST, 0, -32, 32);
        break;
    case 0xE898B54C: // e4m4
        SetWeatherOverride(WEATHERTYPE_DUST, WEATHERTYPE_DUST, 0, 1, 1);
        break;
    case 0xCB7A97D6: // e6m2
        SetWeatherOverride(WEATHERTYPE_RAIN, WEATHERTYPE_DUST, 0, -16, 96);
        break;
    case 0xC3B72664: // e3m7
        SetWeatherOverride(WEATHERTYPE_LAVA, WEATHERTYPE_LAVA, 0, 0, 6);
        break;
    default:
        if (nWeatherOverride)
            UnloadPreset();
        break;
    }
}

void CWeather::UnloadPreset(void)
{
    nWeatherOverride = 0;
    nWeatherOverrideType = WEATHERTYPE_NONE;
    nWeatherOverrideTypeInside = WEATHERTYPE_NONE;
    nWeatherOverrideWindX = 0;
    nWeatherOverrideWindY = 0;
    nWeatherOverrideGravity = 0;
}

void CWeather::SetWeatherOverride(WEATHERTYPE nOverride, WEATHERTYPE nOverrideInside, short nX, short nY, short nZ)
{
    if ((nOverride <= WEATHERTYPE_NONE) || (nOverride >= WEATHERTYPE_MAX)) // invalid, unload
    {
        UnloadPreset();
        return;
    }
    nWeatherOverride = 1;
    nWeatherOverrideType = nOverride;
    nWeatherOverrideTypeInside = nOverrideInside;
    nWeatherOverrideWindX = nX;
    nWeatherOverrideWindY = nY;
    nWeatherOverrideGravity = nZ;
}

inline WEATHERTYPE RandomWeather(unsigned int nRNG)
{
    if (nRNG&0x10)
        return WEATHERTYPE_SNOWHARD;
    else if (nRNG&8)
        return WEATHERTYPE_SNOW;
    else if (nRNG&4)
        return WEATHERTYPE_RAIN;
    else if (nRNG&2)
        return WEATHERTYPE_RAINHARD;
    return WEATHERTYPE_DUST;
}

void CWeather::Process(long nX, long nY, long nZ, int nSector, int nClipDist, unsigned int uMapCRC)
{
    static int nSectorChecked = -1;
    if (nWeatherCheat > WEATHERTYPE_NONE)
    {
        if (nWeatherCur != nWeatherCheat)
            nWeatherCur = WEATHERTYPE_NONE;
        SetWeatherType(nWeatherCheat, uMapCRC);
        nWeatherForecast = nWeatherCheat;
        return;
    }
    if (IsUnderwaterSector(nSector))
    {
        nWeatherForecast = WEATHERTYPE_UNDERWATER; // skip transition if player is underwater
        SetWeatherType(WEATHERTYPE_UNDERWATER, uMapCRC);
        return;
    }
    int tmpSect = nSector;
    if (nSectorChecked != nSector) // moved to new sector, hitscan above
    {
        long ve8, vec, vf0, vf4;
        GetZRangeAtXYZ(nX, nY, nZ, nSector, &vf4, &vf0, &vec, &ve8, nClipDist, 0);
        if ((vf0 & 0xc000) == 0x4000) // we hit ceiling
            tmpSect = vf0 & 0x3ff;
        nSectorChecked = tmpSect;
    }
    if (sector[tmpSect].ceilingstat&1) // outside
        nWeatherForecast = nWeatherOverride ? nWeatherOverrideType : RandomWeather(uMapCRC);
    else // inside
        nWeatherForecast = nWeatherOverride ? nWeatherOverrideTypeInside : WEATHERTYPE_DUST;
    if ((nWeatherCur == WEATHERTYPE_UNDERWATER) && (nWeatherCur != nWeatherForecast)) // if player has just left underwater, skip transition
    {
        nWindXOffset = krand()&0x3fff;
        nWindYOffset = krand()&0x3fff;
        SetWeatherType(nWeatherForecast, uMapCRC);
    }
}

void CWeather::SetWeatherType(WEATHERTYPE nWeather, unsigned int uMapCRC)
{
    if (nWeather == nWeatherCur)
        return;
    nWeatherCur = nWeather;
    switch (nWeather)
    {
    case WEATHERTYPE_RAIN:
        SetTranslucency(2);
        SetGravity(96, 1);
        RandomWind(0, uMapCRC);
        SetColor(128);
        SetColorShift(0);
        SetShape(2);
        SetStaticView(0);
        nLimit = kMaxVectors>>1;
        break;
    case WEATHERTYPE_SNOW:
        SetTranslucency(0);
        SetGravity(32, 1);
        RandomWind(0, uMapCRC);
        SetColor(32);
        SetColorShift(0);
        SetShape(0);
        SetStaticView(0);
        nLimit = kMaxVectors;
        break;
    case WEATHERTYPE_BLOOD:
        SetTranslucency(0);
        SetGravity(48, 1);
        SetWind(0, 0);
        SetColor(152);
        SetColorShift(2);
        SetShape(1);
        SetStaticView(0);
        nLimit = kMaxVectors;
        break;
    case WEATHERTYPE_UNDERWATER:
        SetTranslucency(2);
        SetGravity(1, 1);
        SetWind(0, 0);
        SetColor(170);
        SetColorShift(2);
        SetShape(0);
        SetStaticView(0);
        nLimit = kMaxVectors>>1;
        break;
    case WEATHERTYPE_DUST:
        SetTranslucency(2);
        SetGravity(4, 1);
        SetWind(0, 0);
        SetColor(170);
        SetColorShift(2);
        SetShape(0);
        SetStaticView(0);
        nLimit = kMaxVectors>>2;
        break;
    case WEATHERTYPE_LAVA:
        SetTranslucency(1);
        SetGravity(6, 1);
        SetWind(0, 0);
        SetColor(160);
        SetColorShift(1);
        SetShape(0);
        SetStaticView(0);
        nLimit = kMaxVectors>>3;
        break;
    case WEATHERTYPE_STARS:
        SetTranslucency(1);
        SetGravity(0, 0);
        SetWind(0, 0);
        SetColor(128);
        SetColorShift(0);
        SetShape(0);
        SetStaticView(1);
        nLimit = kMaxVectors;
        break;
    case WEATHERTYPE_RAINHARD:
        SetTranslucency(2);
        SetGravity(128, 1);
        RandomWind(1, uMapCRC);
        SetColor(128);
        SetColorShift(0);
        SetShape(2);
        SetStaticView(0);
        nLimit = kMaxVectors;
        break;
    case WEATHERTYPE_SNOWHARD:
        SetTranslucency(0);
        SetGravity(128, 1);
        RandomWind(1, uMapCRC);
        SetColor(32);
        SetColorShift(0);
        SetShape(1);
        SetStaticView(0);
        nLimit = kMaxVectors-(kMaxVectors>>1);
        break;
    default:
        nDraw.bActive = 0;
        nCount = 0;
        break;
    }
    if (nWeatherOverride && (nWeather == nWeatherOverrideType) && (nWeatherCheat == WEATHERTYPE_NONE)) // apply overrides
    {
        SetWind(nWeatherOverrideWindX, nWeatherOverrideWindY);
        SetGravity(nWeatherOverrideGravity, 1);
    }
}
