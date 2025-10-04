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
#include <memory.h>
#include <stdlib.h>
#include "typedefs.h"
#include "build.h"
#include "db.h"
#include "debug4g.h"
#include "globals.h"
#include "sectorfx.h"
#include "misc.h"
#include "trig.h"
#include "view.h"

byte flicker1[] = {
    0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1
};

byte flicker2[] = {
    1, 2, 4, 2, 3, 4, 3, 2, 0, 0, 1, 2, 4, 3, 2, 0,
    2, 1, 0, 1, 0, 2, 3, 4, 3, 2, 1, 1, 2, 0, 0, 1,
    1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 4, 2, 1, 0, 1,
    0, 0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2
};

byte flicker3[] = {
    4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 2, 4, 3, 4, 4,
    4, 4, 2, 1, 3, 3, 3, 4, 3, 4, 4, 4, 4, 4, 2, 4,
    4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 0, 1,
    0, 1, 0, 1, 0, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 4
};

byte flicker4[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 3, 0, 1, 0, 1, 0, 4, 4, 4, 4, 4, 2, 0,
    0, 0, 0, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 4, 3, 2
};

byte strobe[] = {
    64, 64, 64, 48, 36, 27, 20, 15, 11, 9, 6, 5, 4, 3, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int GetWaveValue(int a, int b, int c)
{
    b &= 2047;
    switch (a)
    {
    case 0:
        return c;
    case 1:
        return (b>>10)*c;
    case 2:
        return (abs(128-(b>>3))*c)>>7;
    case 3:
        return ((b>>3)*c)>>8;
    case 4:
        return ((255-(b>>3))*c)>>8;
    case 5:
        return (c+mulscale30(c,Sin(b)))>>1;
    case 6:
        return flicker1[b>>5]*c;
    case 7:
        return (flicker2[b>>5]*c)>>2;
    case 8:
        return (flicker3[b>>5]*c)>>2;
    case 9:
        return (flicker4[b>>4]*c)>>2;
    case 10:
        return (strobe[b>>5]*c)>>6;
    case 11:
        if (b*4 > 2048)
            return 0;
        return (c-mulscale30(c, Cos(b*4)))>>1;
    }
    return 0;
}

short shadeList[512];
short panList[kMaxXSectors];
int shadeCount;
int panCount;
byte gotsectorROR[(kMaxSectors+7)>>3]; // this is the same as gotsector, except it includes any ROR drawn sectors
byte *pGotsector = gotsectorROR;

short wallPanList[kMaxXWalls];
short wallPanListSect[kMaxXWalls];
short wallPanListNextSect[kMaxXWalls];
int wallPanCount;

void DoSectorLighting(void)
{
    int v4;
    int nXSector;
    int nSector;
    int t1, t2, t3;
    int i;
    int nStartWall;
    int nEndWall;
    int j;
    for (i = 0; i < shadeCount; i++)
    {
        nXSector = shadeList[i];
        XSECTOR *pXSector = &xsector[nXSector];
        nSector = pXSector->reference;
        dassert(sector[nSector].extra == nXSector, 137);
        if (pXSector->at12_0)
        {
            v4 = pXSector->at12_0;
            if (pXSector->at11_5)
            {
                sector[nSector].floorshade = sector[nSector].floorshade - v4;
                if (pXSector->at18_0)
                {
                    int nTemp = pXSector->at33_4;
                    pXSector->at33_4 = sector[nSector].floorpal;
                    sector[nSector].floorpal = nTemp;
                }
            }
            if (pXSector->at11_6)
            {
                sector[nSector].ceilingshade = sector[nSector].ceilingshade - v4;
                if (pXSector->at18_0)
                {
                    int nTemp = pXSector->at1b_4;
                    pXSector->at1b_4 = sector[nSector].ceilingpal;
                    sector[nSector].ceilingpal = nTemp;
                }
            }
            if (pXSector->at11_7)
            {
                nStartWall = sector[nSector].wallptr;
                nEndWall = nStartWall + sector[nSector].wallnum - 1;
                for (j = nStartWall; j <= nEndWall; j++)
                {
                    wall[j].shade = wall[j].shade - v4;
                    if (pXSector->at18_0)
                    {
                        wall[j].pal = sector[nSector].floorpal;
                    }
                }
            }
            pXSector->at12_0 = 0;
        }
        if (pXSector->at11_4 || pXSector->at1_7)
        {
            t1 = pXSector->at11_0;
            t2 = pXSector->atd_6;
            if (!pXSector->at11_4 && pXSector->at1_7)
            {
                t2 = mulscale16(t2, pXSector->at1_7);
            }
            t3 = pXSector->at10_0 * 8;
            v4 = GetWaveValue(t1, t3+pXSector->ate_6*gGameClock, t2);
            if (pXSector->at11_5)
            {
                sector[nSector].floorshade = ClipRange(sector[nSector].floorshade+v4, -128, 127);
                if (pXSector->at18_0 && v4 != 0)
                {
                    int nTemp = pXSector->at33_4;
                    pXSector->at33_4 = sector[nSector].floorpal;
                    sector[nSector].floorpal = nTemp;
                }
            }
            if (pXSector->at11_6)
            {
                sector[nSector].ceilingshade = ClipRange(sector[nSector].ceilingshade+v4, -128, 127);
                if (pXSector->at18_0 && v4 != 0)
                {
                    int nTemp = pXSector->at1b_4;
                    pXSector->at1b_4 = sector[nSector].ceilingpal;
                    sector[nSector].ceilingpal = nTemp;
                }
            }
            if (pXSector->at11_7)
            {
                nStartWall = sector[nSector].wallptr;
                nEndWall = nStartWall + sector[nSector].wallnum - 1;
                for (j = nStartWall; j <= nEndWall; j++)
                {
                    wall[j].shade = ClipRange(wall[j].shade+v4, -128, 127);
                    if (pXSector->at18_0 && v4 != 0)
                    {
                        wall[j].pal = sector[nSector].floorpal;
                    }
                }
            }
            pXSector->at12_0 = v4;
        }
    }
}

void UndoSectorLighting(void)
{
    int i;
    int nXSector;
    int v4;
    int nStartWall;
    int nEndWall;
    int j;
    for (i = 0; i < numsectors; i++)
    {
        nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->at12_0)
            {
                v4 = pXSector->at12_0;
                if (pXSector->at11_5)
                {
                    sector[i].floorshade = sector[i].floorshade - v4;
                    if (pXSector->at18_0)
                    {
                        int nTemp = pXSector->at33_4;
                        pXSector->at33_4 = sector[i].floorpal;
                        sector[i].floorpal = nTemp;
                    }
                }
                if (pXSector->at11_6)
                {
                    sector[i].ceilingshade = sector[i].ceilingshade - v4;
                    if (pXSector->at18_0)
                    {
                        int nTemp = pXSector->at1b_4;
                        pXSector->at1b_4 = sector[i].ceilingpal;
                        sector[i].ceilingpal = nTemp;
                    }
                }
                if (pXSector->at11_7)
                {
                    nStartWall = sector[i].wallptr;
                    nEndWall = nStartWall + sector[i].wallnum - 1;
                    for (j = nStartWall; j <= nEndWall; j++)
                    {
                        wall[j].shade = wall[j].shade - v4;
                        if (pXSector->at18_0)
                        {
                            wall[j].pal = sector[i].floorpal;
                        }
                    }
                }
                pXSector->at12_0 = 0;
            }
        }
    }
}

void DoSectorPanning(void)
{
    const char bDoInterpolation = !VanillaMode() && (gDetail >= 4);
    for (int i = 0; i < panCount; i++)
    {
        int nXSector = panList[i];
        XSECTOR *pXSector = &xsector[nXSector];
        int nSector = pXSector->reference;
        dassert(nSector >= 0 && nSector < kMaxSectors, 308);
        SECTOR *pSector = &sector[nSector];
        dassert(pSector->extra == nXSector, 311);
        if (pXSector->at13_0 || pXSector->at1_7)
        {
            int angle = pXSector->at15_0+1024;
            int speed = pXSector->at14_0<<10;
            if (!pXSector->at13_0 && (pXSector->at1_7&0xffff))
                speed = mulscale16(speed, pXSector->at1_7);

            if (pXSector->at13_1) // Floor
            {
                int nTile = pSector->floorpicnum;
                int px = (pSector->floorxpanning<<8)+pXSector->at32_1;
                int py = (pSector->floorypanning<<8)+pXSector->at34_0;
                if (pSector->floorstat&kSectorStat6)
                    angle -= 512;
                px += mulscale30(speed<<2, Cos(angle))>>((picsiz[nTile]&15)-((pSector->floorstat&kSectorStat3) ? 1 : 0));
                py -= mulscale30(speed<<2, Sin(angle))>>((picsiz[nTile]/16)-((pSector->floorstat&kSectorStat3) ? 1 : 0));
                if (bDoInterpolation && TestBitString(pGotsector, nSector)) // if we can see this sector, interpolate
                    viewInterpolatePanningFloor(nSector, pSector);
                pSector->floorxpanning = px>>8;
                pSector->floorypanning = py>>8;
                pXSector->at32_1 = px&255;
                pXSector->at34_0 = py&255;
            }
            if (pXSector->at13_2) // Ceiling
            {
                int nTile = pSector->ceilingpicnum;
                int px = (pSector->ceilingxpanning<<8)+pXSector->at30_1;
                int py = (pSector->ceilingypanning<<8)+pXSector->at31_1;
                if (pSector->ceilingstat&kSectorStat6)
                    angle -= 512;
                px += mulscale30(speed<<2, Cos(angle))>>((picsiz[nTile]&15)-((pSector->ceilingstat&kSectorStat3) ? 1 : 0));
                py += mulscale30(speed<<2, Sin(angle))>>((picsiz[nTile]/16)-((pSector->ceilingstat&kSectorStat3) ? 1 : 0));
                if (bDoInterpolation && TestBitString(pGotsector, nSector)) // if we can see this sector, interpolate
                    viewInterpolatePanningCeiling(nSector, pSector);
                pSector->ceilingxpanning = px>>8;
                pSector->ceilingypanning = py>>8;
                pXSector->at30_1 = px&255;
                pXSector->at31_1 = py&255;
            }
        }
    }
    for (i = 0; i < wallPanCount; i++)
    {
        int nXWall = wallPanList[i];
        XWALL *pXWall = &xwall[nXWall];
        int nWall = pXWall->reference;
        dassert(wall[nWall].extra == nXWall, 368);
        if (pXWall->atd_6 || pXWall->at1_7)
        {
            int psx = pXWall->atd_7<<10;
            int psy = pXWall->ate_7<<10;
            if (!pXWall->atd_6 && (pXWall->at1_7 & 0xffff))
            {
                psx = mulscale16(psx, pXWall->at1_7);
                psy = mulscale16(psy, pXWall->at1_7);
            }
            int nTile = wall[nWall].picnum;
            int px = (wall[nWall].xpanning<<8)+pXWall->at11_2;
            int py = (wall[nWall].ypanning<<8)+pXWall->at12_2;
            px += (psx<<2)>>(picsiz[nTile]&15);
            py += (psy<<2)>>(picsiz[nTile]/16);
            if (bDoInterpolation && (TestBitString(pGotsector, wallPanListSect[i]) || (wallPanListNextSect[i] >= 0 && TestBitString(pGotsector, wallPanListNextSect[i])))) // if we can see this sector (or the linked sector), interpolate
                viewInterpolatePanningWall(nWall, &wall[nWall]);
            wall[nWall].xpanning = px>>8;
            wall[nWall].ypanning = py>>8;
            pXWall->at11_2 = px&255;
            pXWall->at12_2 = py&255;
        }
    }
}

void InitSectorFX(void)
{
    shadeCount = 0;
    panCount = 0;
    wallPanCount = 0;
    for (int i = 0; i < numsectors; i++)
    {
        short nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->atd_6)
                shadeList[shadeCount++] = nXSector;
            if (pXSector->at14_0)
                panList[panCount++] = nXSector;
        }
    }
    for (i = 0; i < numwalls; i++)
    {
        short nXWall = wall[i].extra;
        if (nXWall > 0)
        {
            XWALL *pXWall = &xwall[nXWall];
            if (pXWall->atd_7 || pXWall->ate_7)
            {
                for (int j = 0; j < numsectors; j++) // lookup sector for wall
                {
                    short startwall = sector[j].wallptr;
                    const short endwall = startwall + sector[j].wallnum;
                    if ((i >= startwall) && (i < endwall))
                    {
                        wallPanListSect[wallPanCount] = j;
                        break;
                    }
                }
                wallPanListNextSect[wallPanCount] = wall[i].nextsector;
                wallPanList[wallPanCount++] = nXWall;
            }
        }
    }
    memset(gotsectorROR, 0, sizeof(gotsectorROR));
    pGotsector = gotsectorROR;
}

static BOOL gHasRenderedROR = FALSE; // only do a OR operation copy if we have rendered a ROR sector

void ClearGotSectorSectorFX(void)
{
    if (VanillaMode() || (gDetail < 4))
        return;
    if (gHasRenderedROR)
    {
        gHasRenderedROR = FALSE;
        memset(gotsectorROR, 0, sizeof(gotsectorROR));
    }
    else
        pGotsector = gotsectorROR;
}

void UpdateGotSectorSectorFX(BOOL bROR)
{
    unsigned long i;
    if (VanillaMode() || (gDetail < 4))
        return;
    if (!gHasRenderedROR) // first run
    {
        if (bROR) // copy from gotsector and return
        {
            gHasRenderedROR = TRUE;
            pGotsector = gotsectorROR;
            memcpy(gotsectorROR, gotsector, sizeof(gotsector));
        }
        else
        {
            pGotsector = gotsector; // we didn't render a ROR surface, don't bother doing compare copy
        }
        return;
    }
    for (i = sizeof(gotsectorROR); i > 3; i -= 4)
    {
        gotsectorROR[i-1] |= gotsector[i-1];
        gotsectorROR[i-2] |= gotsector[i-2];
        gotsectorROR[i-3] |= gotsector[i-3];
        gotsectorROR[i-4] |= gotsector[i-4];
    }
}

class CSectorListMgr
{
public:
    CSectorListMgr();
    int CreateList(short);
    void AddSector(int, short);
    int GetSectorCount(int);
    short *GetSectorList(int);
    int End(int i) { return nListStart[i] + nListSize[i]; };
private:
    int nLists;
    int nListSize[32];
    int nListStart[32];
    short nSectors[kMaxSectors];
};

CSectorListMgr::CSectorListMgr()
{
    nLists = 0;
}

int CSectorListMgr::CreateList(short nSector)
{
    int nStart = 0;
    if (nLists)
    {
        nStart = End(nLists - 1);
    }
    nListStart[nLists] = nStart;
    nListSize[nLists] = 1;
    int vb = nLists;
    nLists++;
    short *pList = GetSectorList(vb);
    pList[0] = nSector;
    return vb;
}

void CSectorListMgr::AddSector(int nList, short nSector)
{
    for (int i = nLists; i > nList; i--)
    {
        short *pList = GetSectorList(i);
        int nCount = GetSectorCount(i);
        memmove(pList+1,pList,nCount*sizeof(short));
        nListStart[i]++;
    }
    short *pList = GetSectorList(nList);
    int nCount = GetSectorCount(nList);
    pList[nCount] = nSector;
    nListSize[nList]++;
}

int CSectorListMgr::GetSectorCount(int nList)
{
    return nListSize[nList];
}

short * CSectorListMgr::GetSectorList(int nList)
{
    return nSectors+nListStart[nList];
}
