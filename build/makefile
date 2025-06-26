#Comment flag to use original A.ASM and disable pentium overlays
!define USE_PENTIUM_ASM
#Comment flag to use cut down cache1d.c (blood's replace.cpp overrides build's original cache1d system)
!define CACHE1D_LEAN_AND_MEAN

SRC_DIR=.\re
OBJ_DIR=$(SRC_DIR)

!ifdef USE_PENTIUM_ASM
#CFLAGSFAST is untested - use at your own risk
#CFLAGSFAST = /omaxtnri /ol+ /oe=32 /5r /zq /zc /D__PENTIUM_ASM__
CFLAGS = /5r /s /orti /D__PENTIUM_ASM__
!else
#Original flags
CFLAGS = /4r /s /or
!endif
!ifdef CACHE1D_LEAN_AND_MEAN
CFLAGS += /DCACHE1D_LEAN_AND_MEAN
!endif

.h: $(SRC_DIR)
.c: $(SRC_DIR)
.asm: $(SRC_DIR)
.obj: $(OBJ_DIR)

OBJS = &
a.obj &
engine.obj &
cache1d.obj

all : engine.lib mmulti.obj .symbolic
	@%null

engine.lib: $(OBJS)
	wlib -n engine.lib -+$(OBJ_DIR)\a.obj -+$(OBJ_DIR)\engine.obj -+$(OBJ_DIR)\cache1d.obj

!ifdef USE_PENTIUM_ASM
a.obj: $(SRC_DIR)\ammx.asm
    wasm $(SRC_DIR)\ammx.asm /fo=$(OBJ_DIR)\a.obj
!else
a.obj: $(SRC_DIR)\a.asm
    masm /DLIBVER_BUILDREV=19960501 $(SRC_DIR)\a.asm $(OBJ_DIR)\a.obj;
!endif

engine.obj: $(SRC_DIR)\engine.c
    wcc386 $(CFLAGS) $(SRC_DIR)\engine.c /DLIBVER_BUILDREV=19961006 /fo=$(OBJ_DIR)\engine.obj

cache1d.obj: $(SRC_DIR)\cache1d.c
    wcc386 $(CFLAGS) $(SRC_DIR)\cache1d.c /DLIBVER_BUILDREV=19961013 /fo=$(OBJ_DIR)\cache1d.obj

mmulti.obj: $(SRC_DIR)\mmulti.c
    wcc386 $(CFLAGS) $(SRC_DIR)\mmulti.c /DLIBVER_BUILDREV=19961213 /fo=mmulti.obj
