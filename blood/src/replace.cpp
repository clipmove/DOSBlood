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
#include "typedefs.h"
#include "build.h"
#include "config.h"
#include "crc32.h"
#include "globals.h"
#include "misc.h"
#include "resource.h"
#include "screen.h"
#include "tile.h"


void uninitcache()
{
}

extern "C" void agecache()
{
}

extern "C" void initcache()
{
}

extern "C" void allocache()
{
}

extern "C" void *kmalloc(long size)
{
    return Resource::Alloc(size);
}

extern "C" void kfree(void *pMem)
{
    Resource::Free(pMem);
}

extern "C" int loadpics()
{
    return tileInit(0, NULL) ? 0 : - 1;
}

extern "C" void loadtile(short nTile)
{
    tileLoadTile(nTile);
}

extern "C" void allocatepermanenttile(short a1, int a2, int a3)
{
    tileAllocTile(a1, a2, a3);
}

void overwritesprite (long thex, long they, short tilenum,
    signed char shade, char stat, char dapalnum)
{
   rotatesprite(thex<<16,they<<16,65536L,(stat&8)<<7,tilenum,shade,dapalnum,
      ((stat&1^1)<<4)+(stat&2)+((stat&4)>>2)+((stat&16)>>2)^((stat&8)>>1),
      windowx1,windowy1,windowx2,windowy2);
}

enum {
    kFakevarFlat = 0x8000,
    kFakevarMask = 0xc000,
};

extern "C" int animateoffs(short a1, ushort a2)
{
    int offset = 0;
    int frames;
    int vd;
    if (a1 < 0 || a1 >= kMaxTiles)
        return offset;
    frames = picanm[a1].animframes;
    if (frames > 0)
    {
        if ((a2&0xc000) == 0x8000)
            vd = (CRC32(&a2, 2)+gFrameClock)>>picanm[a1].animspeed;
        else
            vd = gFrameClock>>picanm[a1].animspeed;
        switch (picanm[a1].animtype)
        {
        case 1:
            offset = vd % (2*frames);
            if (offset >= frames)
                offset = 2*frames-offset;
            break;
        case 2:
            offset = vd % (frames+1);
            break;
        case 3:
            offset = -(vd % (frames+1));
            break;
        }
    }
    return offset;
}

extern "C" void uninitengine()
{
    tileTerm();
}

extern "C" void loadpalette()
{
    scrLoadPalette();
}

extern "C" int getpalookup(int a1, int a2)
{
    if (gFogMode)
        return ClipHigh(a1>>8, 15)*16+ClipRange(a2>>2, 0, 15);
    else
        return ClipRange((a1>>8)+a2, 0, 63);
}

extern "C" void MOUSE_GetDelta(int32 *x, int32 *y);

#if 1 // optimized
#define CONTROL_MouseSensitivity (*(int32 *)(uint32(&CONTROL_JoystickPort)+8U)) // assign offset for static MACT386.LIB variable CONTROL_MouseSensitivity (faster and less safe, use below define if using non-release MACT386.LIB)
const void (*CONTROL_GetMouseDeltaOrig)(int32*, int32*) = (const void(*)(int32*,int32*))(uint32(&CONTROL_GetMouseSensitivity)-0x70); // original function address

void scaleAxis32792(int32 *);
#pragma aux scaleAxis32792 = \
"mov ecx, [ebx]" \
"mov edx, 16764937" \
"mov eax, ecx" \
"imul edx" \
"mov eax, edx" \
"sar eax, 7" \
"sar ecx, 31" \
"sub eax, ecx" \
"mov [ebx], eax" \
parm [ebx] \
modify exact [eax ecx edx]

void CONTROL_GetMouseDeltaNew(int32 *x, int32 *y)
{
    int32 dx, dy;
    static int32 mouse_y_leftover = 0;

    MOUSE_GetDelta(&dx, &dy);
    if (dx != 0)
    {
        // scale up X
        dx <<= 5; // * 32

        // multiply by CONTROL_MouseSensitivity
        dx *= CONTROL_MouseSensitivity;

        // divide by 32792
        scaleAxis32792(&dx);
    }
    *x = dx;

    if (dy != 0)
    {
        // scale up Y
        dy = (dy<<4) + (dy<<5); // * 48

        // multiply by CONTROL_MouseSensitivity
        dy *= CONTROL_MouseSensitivity;

        // divide by 32792
        scaleAxis32792(&dy);
    }

    // add mouse_y_leftover to dy, for sub-pixel accumulation
    dy += mouse_y_leftover;
    *y = dy;

    // save the leftover for next time
    if (dy > 0)
    {
        dy &= 511;
    }
    else
    {
        dy = -dy;
        dy &= 511;
        dy = -dy;
    }
    mouse_y_leftover = dy;
}

void CONTROL_GetMouseDeltaBuildMfx(int32 *x, int32 *y)
{
    int32 dx, dy;
    MOUSE_GetDelta(&dx, &dy);
    dx <<= 5; // * 32
    dy = (dy<<5) + (dy<<6); // * 96
    *x = (dx*CONTROL_MouseSensitivity)>>15;
    *y = (dy*CONTROL_MouseSensitivity)>>15;
}

extern "C" void CONTROL_GetMouseDelta(int32 *x, int32 *y) // this 'hack' ensures the linker will use this function instead of the one in MACT386.LIB
{
    switch (gMouseCalculation)
    {
        case 0: // new (based off of sMouse)
            CONTROL_GetMouseDeltaNew(x, y);
            break;
        case 1: // build mouse fix (https://ctpax-cheater.losthost.org/htmldocs/trouble.htm#buildmfx)
            CONTROL_GetMouseDeltaBuildMfx(x, y);
            break;
        case 2: // original code (use original MACT386.LIB function)
            CONTROL_GetMouseDeltaOrig(x, y);
            break;
        default:
            break;
    }
}
#else // taken from sMouse
#define game_scale_x 32
#define game_scale_y 48
#define game_mouse_y_threshold 512
#define MINIMUMMOUSESENSITIVITY 0x1000
#define CONTROL_MouseSensitivity (gMouseSensitivity+MINIMUMMOUSESENSITIVITY) // static MACT386.LIB variable CONTROL_MouseSensitivity is set by CONTROL_SetMouseSensitivity() and adds MINIMUMMOUSESENSITIVITY to input - recreate the same logic here

static int32 mouse_y_leftover = 0;

extern "C" void CONTROL_GetMouseDelta(int32 *x, int32 *y)
{
    MOUSE_GetDelta(x, y);
    *x *= game_scale_x;
    *y *= game_scale_y;
    *x *= CONTROL_MouseSensitivity;
    *y *= CONTROL_MouseSensitivity;
    *x /= 32792L;
    *y /= 32792L;
#if 0 // MACT later does this in CONTROL_PollDevices()->CONTROL_ScaleAxis(), so don't do this (this is only needed if running as an external program)
    *x *= gMouseAxisSensitivity[0];
    *y *= gMouseAxisSensitivity[1];
    *x /= 65536L;
    *y /= 65536L;
#endif

    *y += mouse_y_leftover;
    if (*y > 0)
        mouse_y_leftover = *y % game_mouse_y_threshold;
    else
        mouse_y_leftover = -(-(*y) % game_mouse_y_threshold);
}
#endif
