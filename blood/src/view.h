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
#ifndef _VIEW_H_
#define _VIEW_H_

#include "typedefs.h"
#include "build.h"
#include "config.h"
#include "controls.h"
#include "misc.h"

struct FONT {
    int tile;
    int xSize;
    int ySize;
    int space;
};

extern FONT gFont[];

extern int gViewMode;
extern int gZoom;
extern int gViewX0S;
extern int gViewX1S;
extern int gViewY0S;
extern int gViewY1S;
extern int gViewIndex;

extern int gShowFrameRate;
extern int gShowLevelLimits;

extern long gScreenTilt;
extern int deliriumTilt;
extern int deliriumPitch;
extern int deliriumTurn;

enum VIEWPOS {
    VIEWPOS_0,
    VIEWPOS_1,
    VIEWPOS_2,
};

extern VIEWPOS gViewPos;

enum INTERPOLATE_TYPE {
    INTERPOLATE_TYPE_INT = 0,
    INTERPOLATE_TYPE_SHORT,
    INTERPOLATE_TYPE_CHAR,
};

struct LOCATION {
    int x, y, z;
    int ang;
};

extern LOCATION gPrevSpriteLoc[kMaxSprites];

extern byte gInterpolateSprite[(kMaxSprites+7)>>3];
extern byte gInterpolateWall[(kMaxWalls+7)>>3];
extern byte gInterpolateSector[(kMaxSectors+7)>>3];
extern byte gInterpolatePanningWall[(kMaxWalls+7)>>3];
extern byte gInterpolatePanningCeiling[(kMaxSectors+7)>>3];
extern byte gInterpolatePanningFloor[(kMaxSectors+7)>>3];

void viewAddInterpolation(void *, INTERPOLATE_TYPE);

inline void viewInterpolateSector(int nSector, SECTOR *pSector)
{
    if (!TestBitString(gInterpolateSector, nSector))
    {
        viewAddInterpolation(&pSector->floorz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->ceilingz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->floorheinum, INTERPOLATE_TYPE_SHORT);
        SetBitString(gInterpolateSector, nSector);
    }
}

inline void viewInterpolateWall(int nWall, WALL *pWall)
{
    if (!TestBitString(gInterpolateWall, nWall))
    {
        viewAddInterpolation(&pWall->x, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pWall->y, INTERPOLATE_TYPE_INT);
        SetBitString(gInterpolateWall, nWall);
    }
}

inline void viewInterpolatePanningWall(int nWall, WALL *pWall)
{
    if (!TestBitString(gInterpolatePanningWall, nWall))
    {
        viewAddInterpolation(&pWall->xpanning, INTERPOLATE_TYPE_CHAR);
        viewAddInterpolation(&pWall->ypanning, INTERPOLATE_TYPE_CHAR);
        SetBitString(gInterpolatePanningWall, nWall);
    }
}

inline void viewInterpolatePanningCeiling(int nSector, SECTOR *pSector)
{
    if (!TestBitString(gInterpolatePanningCeiling, nSector))
    {
        viewAddInterpolation(&pSector->ceilingxpanning, INTERPOLATE_TYPE_CHAR);
        viewAddInterpolation(&pSector->ceilingypanning, INTERPOLATE_TYPE_CHAR);
        SetBitString(gInterpolatePanningCeiling, nSector);
    }
}

inline void viewInterpolatePanningFloor(int nSector, SECTOR *pSector)
{
    if (!TestBitString(gInterpolatePanningFloor, nSector))
    {
        viewAddInterpolation(&pSector->floorxpanning, INTERPOLATE_TYPE_CHAR);
        viewAddInterpolation(&pSector->floorypanning, INTERPOLATE_TYPE_CHAR);
        SetBitString(gInterpolatePanningFloor, nSector);
    }
}

inline void viewBackupSpriteLoc(int nSprite, SPRITE *pSprite)
{
    if (!TestBitString(gInterpolateSprite, nSprite))
    {
        LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
        pPrevLoc->x = pSprite->x;
        pPrevLoc->y = pSprite->y;
        pPrevLoc->z = pSprite->z;
        pPrevLoc->ang = pSprite->ang;
        SetBitString(gInterpolateSprite, nSprite);
    }
}

inline void viewCorrectSpriteInterpolateOffsets(int nSprite, SPRITE *pSprite, VECTOR3D *pOldpos)
{
    if (TestBitString(gInterpolateSprite, nSprite))
    {
        LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
        pPrevLoc->x = pSprite->x+(pPrevLoc->x-pOldpos->dx);
        pPrevLoc->y = pSprite->y+(pPrevLoc->y-pOldpos->dy);
        pPrevLoc->z = pSprite->z+(pPrevLoc->z-pOldpos->dz);
    }
}

void func_1EC78(int, char *, char *, char *);
void viewResizeView(int);
void viewUpdateFov(BOOL bCheck = 0);
void viewToggle(int);
void viewSetMessage(char *);
void viewDrawText(int, char *, int, int, int, int, int position = 0, BOOL shadow = 0);
void viewGetFontInfo(int nFont, char *pString, int *pXSize, int *pYSize);
void viewUpdatePages(void);
void viewDrawSprite(long,long,long,int,int,schar,byte,ushort,long,long,long,long);
void viewInit(void);
void viewBackupView(int);
void viewCorrectViewOffsets(int nPlayer, VECTOR3D *pOldpos);
void viewInitializePrediction(void);
void viewProcessSprites(int cX, int cY, int cZ);
void viewSetErrorMessage(char *);
void viewDrawScreen(void);
void viewUpdateDelirium(void);
void viewClearInterpolations(void);
void viewCorrectPrediction(void);
void viewUpdatePrediction(INPUT *);

#endif // !_VIEW_H_
