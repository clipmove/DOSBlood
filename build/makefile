SRC_DIR=.\re
OBJ_DIR=$(SRC_DIR)

#CFLAGSFAST is untested - use at your own risk
#CFLAGSFAST = /omaxtnri /ol+ /oe=32 /5r /zq /zc -DLIBVER_BUILDREV=19961012 -D__PENTIUM_ASM__
CFLAGS = /5r /s /orti -DLIBVER_BUILDREV=19961012 -D__PENTIUM_ASM__

.h: $(SRC_DIR)
.c: $(SRC_DIR)
.asm: $(SRC_DIR)
.obj: $(OBJ_DIR)

OBJS = &
a.obj &
engine.obj &
cache1d.obj

all : engine.lib .symbolic
	@%null

engine.lib: $(OBJS) mmulti.obj
	wlib engine.lib -+$(OBJ_DIR)\a.obj -+$(OBJ_DIR)\engine.obj -+$(OBJ_DIR)\cache1d.obj

.asm.obj:
    wasm $[* /fo=$(OBJ_DIR)\$^&

.c.obj:
    wcc386 $(CFLAGS) $[* /fo=$(OBJ_DIR)\$^&

mmulti.obj: $(SRC_DIR)\mmulti.c
    wcc386 $(CFLAGS) $(SRC_DIR)\mmulti.c /fo=mmulti.obj
