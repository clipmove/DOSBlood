// *** VERSIONS RESTORATION ***
#ifndef GAMEVER_H
#define GAMEVER_H

// Originally Blood-RE this would allow you to select the build version
// However DOSBlood only supports v1.21, so hardcode it here
#define APPVER_EXEDEF BP121

// It is assumed here that:
// 1. The compiler is set up to appropriately define APPVER_EXEDEF
// as an EXE identifier.
// 2. This header is included (near) the beginning of every compilation unit,
// in order to have an impact in any place where it's expected to.


#define AV_BT_SHAREWARE 0
#define AV_BT_REGISTERED 1
#define AV_BT_PLASMAPAK 2

// APPVER_BLOODREV definitions
#define AV_BR_BL120 199803180 // BLOOD.EXE from 3dfx patch (not to be confused with the 3DFX.EXE)
#define AV_BR_BR120 (AV_BR_BL120+AV_BT_REGISTERED)
#define AV_BR_BP120 (AV_BR_BL120+AV_BT_PLASMAPAK)
#define AV_BR_BL121 199807150 // One Unit Whole Blood
#define AV_BR_BP121 (AV_BR_BL121+AV_BT_PLASMAPAK)

// Now define APPVER_BLOODREV to one of the above, based on APPVER_EXEDEF

#define APPVER_CONCAT1(x,y) x ## y
#define APPVER_CONCAT2(x,y) APPVER_CONCAT1(x,y)
#define APPVER_BLOODREV APPVER_CONCAT2(AV_BR_,APPVER_EXEDEF)

#if (APPVER_BLOODREV%10==AV_BT_SHAREWARE)
#define SHAREWARE
#endif
#if (APPVER_BLOODREV%10==AV_BT_REGISTERED)
#define REGISTERED
#endif
#if (APPVER_BLOODREV%10==AV_BT_PLASMAPAK)
#define REGISTERED
#define PLASMAPAK
#endif

#endif // GAMEVER_H
