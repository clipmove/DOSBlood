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
#ifndef _WEATHER_H_
#define _WEATHER_H_

#include "misc.h"

#define kMaxVectors 4096
#define kScaleTableShift 2 // downscale to save space at the expense of less precision
#define kScaleTableSize (65536>>kScaleTableShift)
#define kScaleTableMask (kScaleTableSize-1)
#define kWeatherTileY 1600 // max res supported

enum WEATHERTYPE {
    WEATHERTYPE_NONE,
    WEATHERTYPE_RAIN,
    WEATHERTYPE_SNOW,
    WEATHERTYPE_BLOOD,
    WEATHERTYPE_UNDERWATER,
    WEATHERTYPE_DUST,
    WEATHERTYPE_LAVA,
    WEATHERTYPE_STARS,
    WEATHERTYPE_RAINHARD,
    WEATHERTYPE_SNOWHARD,
    WEATHERTYPE_MAX,
};

class CWeather {
public:
    CWeather();
    ~CWeather();
    void RandomizeVectors(void);
    void SetViewport(int nX, int nY, int nXOffset0, int nXOffset1, int nYOffset0, int nYOffset1, int nFov);
    void SetParticles(short nCount, short nLimit = -1);
    void SetGravity(short nGravity, char bVariance);
    void SetWind(short nX, short nY);
    void RandomWind(char bHeavyWind);
    void SetTranslucency(int);
    void SetColor(unsigned char a1);
    void SetColorShift(char);
    void SetShape(char);
    void SetStaticView(char);
    void Initialize(int nCount = 0);
    void Draw(char* pBuffer, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int* pYLookup, long nX, long nY, long nZ, int nAng, int nPitch, int nHoriz, int nCount, int nDelta);
    void Draw(long nX, long nY, long nZ, int nAng, int nPitch, int nHoriz, int nCount, long nClock, int nInterpolate);
    void LoadPreset(unsigned int uMapCRC);
    void UnloadPreset(void);
    void SetWeatherOverride(WEATHERTYPE nOverride, WEATHERTYPE nOverrideInside, short nX, short nY, short nZ);
    void Process(long nX, long nY, long nZ, int nSector, int nClipDist);
    void SetWeatherType(WEATHERTYPE nWeather);

    short GetCount(void) {
        return ClipHigh(nCount, nLimit);
    }

    void SetCount(int t) {
        nCount = ClipRange(t, 0, kMaxVectors);
    }

    BOOL Status(void) {
        return nDraw.bActive ? 1 : 0;
    }

    WEATHERTYPE GetWeather(void) {
        return nWeatherCur;
    }

    WEATHERTYPE GetWeatherForecast(void) {
        return nWeatherForecast;
    }

    WEATHERTYPE nWeatherCheat;
private:
    union {
        byte b;
        struct {
            unsigned int bActive : 1;
            unsigned int bStaticView : 1;
            unsigned int bShape : 2;
            unsigned int nTransparent : 2;
            unsigned int bGravityVariance : 1;
        };
    } nDraw;
    int nWidth;
    int nHeight;
    int nOffsetX;
    int nOffsetY;
    int YLookup[kWeatherTileY];
    unsigned int nScaleFactor;
    short nCount;
    short nLimit;
    short nGravity;
    short nWindX;
    short nWindY;
    short nWindXOffset;
    short nWindYOffset;
    short nPos[kMaxVectors][3];
    unsigned char nPalColor;
    char nPalShift;
    int nScaleTableWidth;
    int nScaleTableFov;
    int nFovV;
    int nScaleTable[kScaleTableSize];
    int nLastFrameClock;
    WEATHERTYPE nWeatherCur;
    WEATHERTYPE nWeatherForecast;
    WEATHERTYPE nWeatherOverrideType;
    WEATHERTYPE nWeatherOverrideTypeInside;
    char nWeatherOverride;
    short nWeatherOverrideWindX;
    short nWeatherOverrideWindY;
    short nWeatherOverrideGravity;
};

extern CWeather gWeather;

#endif // !_WEATHER_H_
