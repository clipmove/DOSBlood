; "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
; Ken Silverman's official web site: "http://www.advsys.net/ken"
; See the included license file "BUILDLIC.TXT" for license info.

.586P
.8087
;include mmx.inc       ;Include this if using < WATCOM 11.0 WASM

;Warning: IN THIS FILE, ALL SEGMENTS ARE REMOVED.  THIS MEANS THAT DS:[]
;MUST BE ADDED FOR ALL SELF-MODIFIES FOR MASM TO WORK.
;
;WASM PROBLEMS:
;   1. Requires all scaled registers (*1,*2,*4,*8) to be last thing on line
;   2. Using 'DATA' is nice for self-mod. code, but WDIS only works with 'CODE'
;
;MASM PROBLEMS:
;   1. Requires DS: to be written out for self-modifying code to work
;   2. Doesn't encode short jumps automatically like WASM
;   3. Stupidly adds wait prefix to ffree's

EXTRN _asm1 : dword
EXTRN _asm2 : dword
EXTRN _asm3 : dword
EXTRN _asm4 : dword
EXTRN _reciptable : near
EXTRN _fpuasm : dword
EXTRN _globalx3 : dword
EXTRN _globaly3 : dword
EXTRN _ylookup : near

EXTRN _vplce : near
EXTRN _vince : near
EXTRN _palookupoffse : near
EXTRN _bufplce : near

EXTRN _ebpbak : dword
EXTRN _espbak : dword

EXTRN _pow2char : near
EXTRN _pow2long : near

CODE SEGMENT PUBLIC USE32 'DATA'
ASSUME cs:CODE,ds:CODE

ALIGN 16
PUBLIC sethlinesizes_
sethlinesizes_:
	mov byte ptr [machxbits1+2], al
	mov byte ptr [machxbits2+2], al
	mov byte ptr [machxbits3+2], al
	neg al
	mov byte ptr [hxsiz1+2], al
	mov byte ptr [hxsiz2+2], al
	mov byte ptr [hxsiz3+2], al
	mov byte ptr [hxsiz4+2], al
	mov byte ptr [machnegxbits1+2], al

	mov byte ptr [hysiz1+3], bl
	mov byte ptr [hysiz2+3], bl
	mov byte ptr [hysiz3+3], bl
	mov byte ptr [hysiz4+3], bl
	mov byte ptr [hmach3a+2], bl
	mov byte ptr [hmach3b+2], bl
	mov byte ptr [hmach3c+2], bl
	mov byte ptr [hmach3d+2], bl

	mov dword ptr [hoffs1+2], ecx
	mov dword ptr [hoffs2+2], ecx
	mov dword ptr [hoffs3+2], ecx
	mov dword ptr [hoffs4+2], ecx
	mov dword ptr [hoffs5+2], ecx
	mov dword ptr [hoffs6+2], ecx
	mov dword ptr [hoffs7+2], ecx
	mov dword ptr [hoffs8+2], ecx

	mov edx, -1
	mov cl, al
	sub cl, bl
	shr edx, cl
	mov dword ptr [hmach2a+1], edx
	mov dword ptr [hmach2b+1], edx
	mov dword ptr [hmach2c+1], edx
	mov dword ptr [hmach2d+1], edx

	ret

ALIGN 16
PUBLIC prosethlinesizes_
prosethlinesizes_:
	mov dword ptr [prohbuf-4], ecx
	neg eax
	mov ecx, eax
	sub eax, ebx
	mov byte ptr [prohshru-1], al   ;bl = 32-al-bl
	mov eax, -1
	shr eax, cl
	mov ecx, ebx
	shl eax, cl
	mov dword ptr [prohand-4], eax  ;((-1>>(-oal))<<obl)
	neg ebx
	mov byte ptr [prohshrv-1], bl   ;bl = 32-bl
	ret

ALIGN 16
PUBLIC setvlinebpl_
setvlinebpl_:
	mov dword ptr [fixchain1a+2], eax
	mov dword ptr [fixchain1b+2], eax
	mov dword ptr [fixchain1m+2], eax
	mov dword ptr [fixchain1t+2], eax
	mov dword ptr [fixchain1s+2], eax
	mov dword ptr [mfixchain1s+2], eax
	mov dword ptr [tfixchain1s+2], eax
	mov dword ptr [fixchain2a+2], eax
	mov dword ptr [profixchain2a+2], eax
	mov dword ptr [fixchain2ma+2], eax
	mov dword ptr [fixchain2mb+2], eax
	mov dword ptr [fixchaint2a+1], eax
	mov dword ptr [fixchaint2b+2], eax
	mov dword ptr [fixchaint2c+2], eax
	mov dword ptr [fixchaint2d+2], eax
	mov dword ptr [fixchaint2e+2], eax
	ret

ALIGN 16
PUBLIC setpalookupaddress_
setpalookupaddress_:
	mov dword ptr [pal1+2], eax
	mov dword ptr [pal2+2], eax
	mov dword ptr [pal3+2], eax
	mov dword ptr [pal4+2], eax
	mov dword ptr [pal5+2], eax
	mov dword ptr [pal6+2], eax
	mov dword ptr [pal7+2], eax
	mov dword ptr [pal8+2], eax
	ret

ALIGN 16
PUBLIC prosetpalookupaddress_
prosetpalookupaddress_:
	mov dword ptr [prohpala-4], eax
	ret

ALIGN 16
PUBLIC setuphlineasm4_
setuphlineasm4_:
machxbits3: rol eax, 6                     ;xbits
	mov dword ptr [hmach4a+2], eax
	mov dword ptr [hmach4b+2], eax
	mov bl, al
	mov dword ptr [hmach4c+2], eax
	mov dword ptr [hmach4d+2], eax
	mov dword ptr [hmach1a+2], ebx
	mov dword ptr [hmach1b+2], ebx
	mov dword ptr [hmach1c+2], ebx
	mov dword ptr [hmach1d+2], ebx
	ret

	;Non-256-stuffed ceiling&floor method with NO SHLD!:
	;yinc&0xffffff00   lea eax, [edx+88888800h]           1     1/2
	;ybits...xbits     and edx, 88000088h                 1     1/2
	;ybits             rol edx, 6                         2     1/2
	;xinc<<xbits       add esi, 88888888h                 1     1/2
	;xinc>>(32-xbits)  adc al, 88h                        1     1/2
	;bufplc            mov cl, byte ptr [edx+88888888h]   1     1/2
	;paloffs&255       mov bl, byte ptr [ecx+88888888h]   1     1/2
ALIGN 16
PUBLIC hlineasm4_
hlineasm4_:
	push ebp

	lea ebp, [eax+1]

	cmp ebp, 8
	jle shorthline

	test edi, 1
	jnz short skipthe1byte

	mov eax, esi
hxsiz1: shr eax, 26
hysiz1: shld eax, edx, 6
hoffs1: mov cl, byte ptr [eax+88888888h]
pal1: mov bl, byte ptr [ecx+88888888h]
	sub esi, _asm1
	sub edx, _asm2
	mov byte ptr [edi], bl
	dec edi
	dec ebp

skipthe1byte:
	test edi, 2
	jnz short skipthe2byte

	mov eax, esi
hxsiz2: shr eax, 26
hysiz2: shld eax, edx, 6
hoffs2: mov cl, byte ptr [eax+88888888h]
pal2: mov bh, byte ptr [ecx+88888888h]
	sub esi, _asm1
	sub edx, _asm2

	mov eax, esi
hxsiz3: shr eax, 26
hysiz3: shld eax, edx, 6
hoffs3: mov cl, byte ptr [eax+88888888h]
pal3: mov bl, byte ptr [ecx+88888888h]
	sub esi, _asm1
	sub edx, _asm2
	mov word ptr [edi-1], bx
	sub edi, 2
	sub ebp, 2

skipthe2byte:

	mov eax, esi
machxbits1: shl esi, 6                     ;xbits
machnegxbits1: shr eax, 32-6               ;32-xbits
	mov dl, al

	inc edi

	add ebx, ebx
	mov eax, edx
	jc beginhline64

	mov eax, _asm1
machxbits2: rol eax, 6                     ;xbits
	mov dword ptr [hmach4a+2], eax
	mov dword ptr [hmach4b+2], eax
	mov dword ptr [hmach4c+2], eax
	mov dword ptr [hmach4d+2], eax
	mov ebx, eax
	mov eax, _asm2
	mov al, bl
	mov dword ptr [hmach1a+2], eax
	mov dword ptr [hmach1b+2], eax
	mov dword ptr [hmach1c+2], eax
	mov dword ptr [hmach1d+2], eax

	mov eax, edx
	jmp beginhline64
ALIGN 16
prebeginhline64:
	mov dword ptr [edi], ebx
beginhline64:

hmach3a: rol eax, 6
hmach2a: and eax, 00008888h
hmach4a: sub esi, 88888888h
hmach1a: sbb edx, 88888888h
	sub edi, 4
hoffs4: mov cl, byte ptr [eax+88888888h]
	mov eax, edx

hmach3b: rol eax, 6
hmach2b: and eax, 00008888h
hmach4b: sub esi, 88888888h
hmach1b: sbb edx, 88888888h
pal4: mov bh, byte ptr [ecx+88888888h]
hoffs5: mov cl, byte ptr [eax+88888888h]
	mov eax, edx

hmach3c: rol eax, 6
pal5: mov bl, byte ptr [ecx+88888888h]
hmach2c: and eax, 00008888h
	shl ebx, 16
hmach4c: sub esi, 88888888h
hmach1c: sbb edx, 88888888h
hoffs6: mov cl, byte ptr [eax+88888888h]

	mov eax, edx
	;(

hmach3d: rol eax, 6
hmach2d: and eax, 00008888h
hmach4d: sub esi, 88888888h
hmach1d: sbb edx, 88888888h
pal6: mov bh, byte ptr [ecx+88888888h]
hoffs7: mov cl, byte ptr [eax+88888888h]
	mov eax, edx
	sub ebp, 4
	nop
pal7: mov bl, byte ptr [ecx+88888888h]
	jnc prebeginhline64
skipthe4byte:

	test ebp, 2
	jz skipdrawthe2
	rol ebx, 16
	mov word ptr [edi+2], bx
	sub edi, 2
skipdrawthe2:
	test ebp, 1
	jz skipdrawthe1
	shr ebx, 24
	mov byte ptr [edi+3], bl
skipdrawthe1:

	pop ebp
	ret

shorthline:
	test ebp, ebp
	jz endshorthline
begshorthline:
	mov eax, esi
hxsiz4: shr eax, 26
hysiz4: shld eax, edx, 6
hoffs8: mov cl, byte ptr [eax+88888888h]
pal8: mov bl, byte ptr [ecx+88888888h]
	sub esi, _asm1
	sub edx, _asm2
	mov byte ptr [edi], bl
	dec edi
	dec ebp
	jnz begshorthline
endshorthline:
	pop ebp
	ret


	;eax: 00000000 00000000 00000000 temp----
	;ebx: 00000000 00000000 00000000 temp----
	;ecx: UUUUUUuu uuuuuuuu uuuuuuuu uuuuuuuu
	;edx: VVVVVVvv vvvvvvvv vvvvvvvv vvvvvvvv
	;esi: cnt----- -------- -------- --------
	;edi: vid----- -------- -------- --------
	;ebp: paloffs- -------- -------- --------
	;esp: ???????? ???????? ???????? ????????
ALIGN 16
PUBLIC prohlineasm4_
prohlineasm4_:
	push ebp

	lea ebp, [ecx+88888888h]
prohpala:
	mov ecx, esi
	lea esi, [eax+1]
	sub edi, esi

prohbeg:
	mov eax, ecx
	shr eax, 20
prohshru:
	mov ebx, edx
	shr ebx, 26
prohshrv:
	and eax, 88888888h
prohand:
	movzx eax, byte ptr [eax+ebx+88888888h]
prohbuf:
	mov al, [eax+ebp]
	sub ecx, _asm1
	sub edx, _asm2
	mov [edi+esi], al
	dec esi
	jnz prohbeg

	pop ebp
	ret



ALIGN 16
PUBLIC setupvlineasm_
setupvlineasm_:
		;First 2 lines for VLINEASM1, rest for VLINEASM4
	mov byte ptr [premach3a+2], al
	mov byte ptr [mach3a+2], al

	push ecx
	mov byte ptr [machvsh1+2], al      ;32-shy
	mov byte ptr [machvsh3+2], al      ;32-shy
	mov byte ptr [machvsh5+2], al      ;32-shy
	mov byte ptr [machvsh6+2], al      ;32-shy
	mov ah, al
	sub ah, 16
	mov byte ptr [machvsh8+2], ah      ;16-shy
	neg al
	mov byte ptr [machvsh7+2], al      ;shy
	mov byte ptr [machvsh9+2], al      ;shy
	mov byte ptr [machvsh10+2], al     ;shy
	mov byte ptr [machvsh11+2], al     ;shy
	mov byte ptr [machvsh12+2], al     ;shy
	mov cl, al
	mov eax, 1
	shl eax, cl
	dec eax
	mov dword ptr [machvsh2+2], eax    ;(1<<shy)-1
	mov dword ptr [machvsh4+2], eax    ;(1<<shy)-1
	pop ecx
	ret

ALIGN 16
PUBLIC prosetupvlineasm_
prosetupvlineasm_:
		;First 2 lines for VLINEASM1, rest for VLINEASM4
	mov byte ptr [premach3a+2], al
	mov byte ptr [mach3a+2], al

	push ecx
	mov byte ptr [promachvsh1+2], al      ;32-shy
	mov byte ptr [promachvsh3+2], al      ;32-shy
	mov byte ptr [promachvsh5+2], al      ;32-shy
	mov byte ptr [promachvsh6+2], al      ;32-shy
	mov ah, al
	sub ah, 16
	mov byte ptr [promachvsh8+2], ah      ;16-shy
	neg al
	mov byte ptr [promachvsh7+2], al      ;shy
	mov byte ptr [promachvsh9+2], al      ;shy
	mov byte ptr [promachvsh10+2], al     ;shy
	mov byte ptr [promachvsh11+2], al     ;shy
	mov byte ptr [promachvsh12+2], al     ;shy
	mov cl, al
	mov eax, 1
	shl eax, cl
	dec eax
	mov dword ptr [promachvsh2+2], eax    ;(1<<shy)-1
	mov dword ptr [promachvsh4+2], eax    ;(1<<shy)-1
	pop ecx
	ret

ALIGN 16
PUBLIC setupmvlineasm_
setupmvlineasm_:
	mov byte ptr [maskmach3a+2], al
	mov byte ptr [machmv13+2], al
	mov byte ptr [machmv14+2], al
	mov byte ptr [machmv15+2], al
	mov byte ptr [machmv16+2], al
	ret

ALIGN 16
PUBLIC setuptvlineasm_
setuptvlineasm_:
	mov byte ptr [transmach3a+2], al
	ret

ALIGN 16
PUBLIC prevlineasm1_
prevlineasm1_:
	test ecx, ecx
	jnz vlineasm1_

	add eax, edx
premach3a: shr edx, 32
	mov dl, byte ptr [esi+edx]
	mov cl, byte ptr [ebx+edx]
	mov byte ptr [edi], cl
	ret

ALIGN 16
PUBLIC vlineasm1_
vlineasm1_:
	push ebp
	mov ebp, ebx
	inc ecx
fixchain1a: sub edi, 320
beginvline:
	mov ebx, edx
mach3a: shr ebx, 32
fixchain1b: add edi, 320
	mov bl, byte ptr [esi+ebx]
	add edx, eax
	dec ecx
	mov bl, byte ptr [ebp+ebx]
	mov byte ptr [edi], bl
	jnz short beginvline
	pop ebp
	mov eax, edx
	ret

ALIGN 16
PUBLIC mvlineasm1_      ;Masked vline
mvlineasm1_:
	push ebp
	mov ebp, ebx
beginmvline:
	mov ebx, edx
maskmach3a: shr ebx, 32
	mov bl, byte ptr [esi+ebx]
	cmp bl, 255
	je short skipmask1
maskmach3c: mov bl, [ebp+ebx]
	mov byte ptr [edi], bl
skipmask1:
	add edx, eax
fixchain1m: add edi, 320
	sub ecx, 1
	jnc short beginmvline

	pop ebp
	mov eax, edx
	ret

ALIGN 16
PUBLIC fixtransluscence_
fixtransluscence_:
	mov dword ptr [transmach4+2], eax
	mov dword ptr [tmach1+2], eax
	mov dword ptr [tmach2+2], eax
	mov dword ptr [tmach3+2], eax
	mov dword ptr [tmach4+2], eax
	mov dword ptr [tran2traa+2], eax
	mov dword ptr [tran2trab+2], eax
	mov dword ptr [tran2trac+2], eax
	mov dword ptr [tran2trad+2], eax
	ret

ALIGN 16
PUBLIC settransnormal_
settransnormal_:
	mov byte ptr [transrev0+1], 83h
	mov byte ptr [transrev1+1], 27h
	mov byte ptr [transrev2+1], 3fh
	mov byte ptr [transrev3+1], 98h
	mov byte ptr [transrev4+1], 90h
	mov byte ptr [transrev5+1], 37h
	mov byte ptr [transrev6+1], 90h
	mov word ptr [transrev7+0], 0f38ah
	mov byte ptr [transrev8+1], 90h
	mov word ptr [transrev9+0], 0f78ah
	mov byte ptr [transrev10+1], 0a7h
	mov byte ptr [transrev11+1], 81h
	mov byte ptr [transrev12+2], 9fh
	mov word ptr [transrev13+0], 0dc88h
	mov byte ptr [transrev14+1], 81h
	mov byte ptr [transrev15+1], 9ah
	mov byte ptr [transrev16+1], 0a7h
	mov byte ptr [transrev17+1], 82h
	ret

ALIGN 16
PUBLIC settransreverse_
settransreverse_:
	mov byte ptr [transrev0+1], 0a3h
	mov byte ptr [transrev1+1], 7h
	mov byte ptr [transrev2+1], 1fh
	mov byte ptr [transrev3+1], 0b8h
	mov byte ptr [transrev4+1], 0b0h
	mov byte ptr [transrev5+1], 17h
	mov byte ptr [transrev6+1], 0b0h
	mov word ptr [transrev7+0], 0d38ah
	mov byte ptr [transrev8+1], 0b0h
	mov word ptr [transrev9+0], 0d78ah
	mov byte ptr [transrev10+1], 87h
	mov byte ptr [transrev11+1], 0a1h
	mov byte ptr [transrev12+2], 87h
	mov word ptr [transrev13+0], 0e388h
	mov byte ptr [transrev14+1], 0a1h
	mov byte ptr [transrev15+1], 0bah
	mov byte ptr [transrev16+1], 87h
	mov byte ptr [transrev17+1], 0a2h
	ret

ALIGN 16
PUBLIC tvlineasm1_        ;Masked & transluscent vline
tvlineasm1_:
	push ebp
	mov ebp, eax
	xor eax, eax
	inc ecx
	mov dword ptr [transmach3c+2], ebx
	jmp short begintvline
ALIGN 16
begintvline:
	mov ebx, edx
transmach3a: shr ebx, 32
	mov bl, byte ptr [esi+ebx]
	cmp bl, 255
	je short skiptrans1
transrev0:
transmach3c: mov al, [ebx+88888888h]
transrev1:
	mov ah, byte ptr [edi]
transmach4: mov al, byte ptr [eax+88888888h]   ;_transluc[eax]
	mov byte ptr [edi], al
skiptrans1:
	add edx, ebp
fixchain1t: add edi, 320
	dec ecx
	jnz short begintvline

	pop ebp
	mov eax, edx
	ret

	;eax: -------temp1-------
	;ebx: -------temp2-------
	;ecx:  dat  dat  dat  dat
	;edx: ylo2           ylo4
	;esi: yhi1           yhi2
	;edi: ---videoplc/cnt----
	;ebp: yhi3           yhi4
	;esp:
ALIGN 16
PUBLIC vlineasm4_
vlineasm4_:
	push ebp

	mov eax, dword ptr _ylookup[ecx*4]
	add eax, edi
	mov dword ptr [machvline4end+2], eax
	sub edi, eax

	mov eax, dword ptr _bufplce[0]
	mov ebx, dword ptr _bufplce[4]
	mov ecx, dword ptr _bufplce[8]
	mov edx, dword ptr _bufplce[12]
	mov dword ptr [machvbuf1+2], ecx
	mov dword ptr [machvbuf2+2], edx
	mov dword ptr [machvbuf3+2], eax
	mov dword ptr [machvbuf4+2], ebx

	mov eax, dword ptr _palookupoffse[0]
	mov ebx, dword ptr _palookupoffse[4]
	mov ecx, dword ptr _palookupoffse[8]
	mov edx, dword ptr _palookupoffse[12]
	mov dword ptr [machvpal1+2], ecx
	mov dword ptr [machvpal2+2], edx
	mov dword ptr [machvpal3+2], eax
	mov dword ptr [machvpal4+2], ebx

		;     +---------------+---------------+
		;edx: |v3lo           |v1lo           |
		;     +---------------+-------+-------+
		;esi: |v2hi  v2lo             |   v3hi|
		;     +-----------------------+-------+
		;ebp: |v0hi  v0lo             |   v1hi|
		;     +-----------------------+-------+

	mov ebp, dword ptr _vince[0]
	mov ebx, dword ptr _vince[4]
	mov esi, dword ptr _vince[8]
	mov eax, dword ptr _vince[12]
	and esi, 0fffffe00h
	and ebp, 0fffffe00h
machvsh9: rol eax, 88h                 ;sh
machvsh10: rol ebx, 88h                ;sh
	mov edx, eax
	mov ecx, ebx
	shr ecx, 16
	and edx, 0ffff0000h
	add edx, ecx
	and eax, 000001ffh
	and ebx, 000001ffh
	add esi, eax
	add ebp, ebx
	;
	mov eax, edx
	and eax, 0ffff0000h
	mov dword ptr [machvinc1+2], eax
	mov dword ptr [machvinc2+2], esi
	mov byte ptr [machvinc3+2], dl
	mov byte ptr [machvinc4+2], dh
	mov dword ptr [machvinc5+2], ebp

	mov ebp, dword ptr _vplce[0]
	mov ebx, dword ptr _vplce[4]
	mov esi, dword ptr _vplce[8]
	mov eax, dword ptr _vplce[12]
	and esi, 0fffffe00h
	and ebp, 0fffffe00h
machvsh11: rol eax, 88h                ;sh
machvsh12: rol ebx, 88h                ;sh
	mov edx, eax
	mov ecx, ebx
	shr ecx, 16
	and edx, 0ffff0000h
	add edx, ecx
	and eax, 000001ffh
	and ebx, 000001ffh
	add esi, eax
	add ebp, ebx

	mov ecx, esi
	jmp short beginvlineasm4
ALIGN 16
	nop
	nop
	nop
beginvlineasm4:
machvsh1: shr ecx, 88h          ;32-sh
	mov ebx, esi
machvsh2: and ebx, 00000088h    ;(1<<sh)-1
machvinc1: add edx, 88880000h
machvinc2: adc esi, 88888088h
machvbuf1: mov cl, byte ptr [ecx+88888888h]
machvbuf2: mov bl, byte ptr [ebx+88888888h]
	mov eax, ebp
machvsh3: shr eax, 88h          ;32-sh
machvpal1: mov cl, byte ptr [ecx+88888888h]
machvpal2: mov ch, byte ptr [ebx+88888888h]
	mov ebx, ebp
	shl ecx, 16
machvsh4: and ebx, 00000088h    ;(1<<sh)-1
machvinc3: add dl, 88h
machvbuf3: mov al, byte ptr [eax+88888888h]
machvinc4: adc dh, 88h
machvbuf4: mov bl, byte ptr [ebx+88888888h]
machvinc5: adc ebp, 88888088h
machvpal3: mov cl, byte ptr [eax+88888888h]
machvpal4: mov ch, byte ptr [ebx+88888888h]
machvline4end: mov dword ptr [edi+88888888h], ecx
fixchain2a: add edi, 88888888h
	mov ecx, esi
	jnc short beginvlineasm4

		;     +---------------+---------------+
		;edx: |v3lo           |v1lo           |
		;     +---------------+-------+-------+
		;esi: |v2hi  v2lo             |   v3hi|
		;     +-----------------------+-------+
		;ebp: |v0hi  v0lo             |   v1hi|
		;     +-----------------------+-------+

	mov dword ptr _vplce[8], esi
	mov dword ptr _vplce[0], ebp
		;vplc2 = (esi<<(32-sh))+(edx>>sh)
		;vplc3 = (ebp<<(32-sh))+((edx&65535)<<(16-sh))
machvsh5: shl esi, 88h     ;32-sh
	mov eax, edx
machvsh6: shl ebp, 88h     ;32-sh
	and edx, 0000ffffh
machvsh7: shr eax, 88h     ;sh
	add esi, eax
machvsh8: shl edx, 88h     ;16-sh
	add ebp, edx
	mov dword ptr _vplce[12], esi
	mov dword ptr _vplce[4], ebp

	pop ebp
	ret

	;eax: -------temp1-------
	;ebx: -------temp2-------
	;ecx: ylo4      ---------
	;edx: ylo2      ---------
	;esi: yhi1           yhi2
	;edi: ---videoplc/cnt----
	;ebp: yhi3           yhi4
	;esp:
ALIGN 16
PUBLIC provlineasm4_
provlineasm4_:
	push ebp

	mov eax, dword ptr _ylookup[ecx*4]
	add eax, edi
	mov dword ptr [promachvline4end1+2], eax
	inc eax
	mov dword ptr [promachvline4end2+2], eax
	inc eax
	mov dword ptr [promachvline4end3+2], eax
	inc eax
	mov dword ptr [promachvline4end4+2], eax
	sub eax, 3
	sub edi, eax

	mov eax, dword ptr _bufplce[0]
	mov ebx, dword ptr _bufplce[4]
	mov ecx, dword ptr _bufplce[8]
	mov edx, dword ptr _bufplce[12]
	mov dword ptr [promachvbuf1+3], ecx
	mov dword ptr [promachvbuf2+3], edx
	mov dword ptr [promachvbuf3+3], eax
	mov dword ptr [promachvbuf4+3], ebx

	mov eax, dword ptr _palookupoffse[0]
	mov ebx, dword ptr _palookupoffse[4]
	mov ecx, dword ptr _palookupoffse[8]
	mov edx, dword ptr _palookupoffse[12]
	mov dword ptr [promachvpal1+2], ecx
	mov dword ptr [promachvpal2+2], edx
	mov dword ptr [promachvpal3+2], eax
	mov dword ptr [promachvpal4+2], ebx

		;     +---------------+---------------+
		;edx: |v3lo           |v1lo           |
		;     +---------------+-------+-------+
		;esi: |v2hi  v2lo             |   v3hi|
		;     +-----------------------+-------+
		;ebp: |v0hi  v0lo             |   v1hi|
		;     +-----------------------+-------+

	mov ebp, dword ptr _vince[0]
	mov ebx, dword ptr _vince[4]
	mov esi, dword ptr _vince[8]
	mov eax, dword ptr _vince[12]
	and esi, 0fffffe00h
	and ebp, 0fffffe00h
promachvsh9: rol eax, 88h                 ;sh
promachvsh10: rol ebx, 88h                ;sh
	mov edx, eax
	mov ecx, ebx
	shr ecx, 16
	and edx, 0ffff0000h
	add edx, ecx
	and eax, 000001ffh
	and ebx, 000001ffh
	add esi, eax
	add ebp, ebx
	;
	mov eax, edx
	and eax, 0ffff0000h
	mov dword ptr [promachvinc1+2], eax
	mov dword ptr [promachvinc2+2], esi
	shl edx, 16
	mov dword ptr [promachvinc3+2], edx
	mov dword ptr [promachvinc5+2], ebp

	mov ebp, dword ptr _vplce[0]
	mov ebx, dword ptr _vplce[4]
	mov esi, dword ptr _vplce[8]
	mov eax, dword ptr _vplce[12]
	and esi, 0fffffe00h
	and ebp, 0fffffe00h
promachvsh11: rol eax, 88h                ;sh
promachvsh12: rol ebx, 88h                ;sh
	mov edx, eax
	mov ecx, ebx
	shr ecx, 16
	and edx, 0ffff0000h
	add edx, ecx
	and eax, 000001ffh
	and ebx, 000001ffh
	add esi, eax
	add ebp, ebx

	mov eax, esi
	mov ecx, edx
	shl ecx, 16
	jmp short probeginvlineasm4
ALIGN 16
	nop
	nop
	nop
probeginvlineasm4:
promachvsh1: shr eax, 88h          ;32-sh
	mov ebx, esi
promachvsh2: and ebx, 00000088h    ;(1<<sh)-1
promachvinc1: add edx, 88880000h
promachvinc2: adc esi, 88888088h
promachvbuf1: movzx eax, byte ptr [eax+88888888h]
promachvbuf2: movzx ebx, byte ptr [ebx+88888888h]
promachvpal1: mov al, byte ptr [eax+88888888h]
promachvline4end3: mov byte ptr [edi+88888888h], al
	mov eax, ebp
promachvsh3: shr eax, 88h          ;32-sh
promachvpal2: mov bl, byte ptr [ebx+88888888h]
promachvline4end4: mov byte ptr [edi+88888888h], bl
	mov ebx, ebp
promachvsh4: and ebx, 00000088h    ;(1<<sh)-1
promachvbuf3: movzx eax, byte ptr [eax+88888888h]
promachvinc3: add ecx, 88888888h
promachvbuf4: movzx ebx, byte ptr [ebx+88888888h]
promachvinc5: adc ebp, 88888088h
promachvpal3: mov al, byte ptr [eax+88888888h]
promachvline4end1: mov byte ptr [edi+88888888h], al
promachvpal4: mov bl, byte ptr [ebx+88888888h]
promachvline4end2: mov byte ptr [edi+88888888h], bl
profixchain2a: add edi, 88888888h
	mov eax, esi
	jnc short probeginvlineasm4

		;     +---------------+---------------+
		;edx: |v3lo           |v1lo           |
		;     +---------------+-------+-------+
		;esi: |v2hi  v2lo             |   v3hi|
		;     +-----------------------+-------+
		;ebp: |v0hi  v0lo             |   v1hi|
		;     +-----------------------+-------+

	mov dword ptr _vplce[8], esi
	mov dword ptr _vplce[0], ebp
		;vplc2 = (esi<<(32-sh))+(edx>>sh)
		;vplc3 = (ebp<<(32-sh))+((edx&65535)<<(16-sh))
promachvsh5: shl esi, 88h     ;32-sh
	mov eax, edx
promachvsh6: shl ebp, 88h     ;32-sh
	and edx, 0000ffffh
promachvsh7: shr eax, 88h     ;sh
	add esi, eax
promachvsh8: shl edx, 88h     ;16-sh
	add ebp, edx
	mov dword ptr _vplce[12], esi
	mov dword ptr _vplce[4], ebp

	pop ebp
	ret


ALIGN 16
PUBLIC mvlineasm4_
mvlineasm4_:
	push ebp

	mov eax, dword ptr _bufplce[0]
	mov ebx, dword ptr _bufplce[4]
	mov dword ptr [machmv1+2], eax
	mov dword ptr [machmv4+2], ebx
	mov eax, dword ptr _bufplce[8]
	mov ebx, dword ptr _bufplce[12]
	mov dword ptr [machmv7+2], eax
	mov dword ptr [machmv10+2], ebx

	mov eax, dword ptr _palookupoffse[0]
	mov ebx, dword ptr _palookupoffse[4]
	mov dword ptr [machmv2+2], eax
	mov dword ptr [machmv5+2], ebx
	mov eax, dword ptr _palookupoffse[8]
	mov ebx, dword ptr _palookupoffse[12]
	mov dword ptr [machmv8+2], eax
	mov dword ptr [machmv11+2], ebx

	mov eax, dword ptr _vince[0]        ;vince
	mov ebx, dword ptr _vince[4]
	xor al, al
	xor bl, bl
	mov dword ptr [machmv3+2], eax
	mov dword ptr [machmv6+2], ebx
	mov eax, dword ptr _vince[8]
	mov ebx, dword ptr _vince[12]
	mov dword ptr [machmv9+2], eax
	mov dword ptr [machmv12+2], ebx

	mov ebx, ecx
	mov ecx, dword ptr _vplce[0]
	mov edx, dword ptr _vplce[4]
	mov esi, dword ptr _vplce[8]
	mov ebp, dword ptr _vplce[12]
	mov cl, bl
	inc cl
	inc bh
	mov byte ptr _asm3[0], bh
fixchain2ma: sub edi, 320

	jmp short beginmvlineasm4
ALIGN 16
beginmvlineasm4:
	dec cl
	jz endmvlineasm4
beginmvlineasm42:
	mov eax, ebp
	mov ebx, esi
machmv16: shr eax, 32
machmv15: shr ebx, 32
machmv12: add ebp, 88888888h ;vince[3]
machmv9: add esi, 88888888h ;vince[2]
machmv10: mov al, byte ptr [eax+88888888h] ;bufplce[3]
machmv7: mov bl, byte ptr [ebx+88888888h] ;bufplce[2]
	cmp al, 255
	adc dl, dl
	cmp bl, 255
	adc dl, dl
machmv8: mov bl, byte ptr [ebx+88888888h] ;palookupoffs[2]
machmv11: mov bh, byte ptr [eax+88888888h] ;palookupoffs[3]

	mov eax, edx
machmv14: shr eax, 32
	shl ebx, 16
machmv4: mov al, byte ptr [eax+88888888h] ;bufplce[1]
	cmp al, 255
	adc dl, dl
machmv6: add edx, 88888888h ;vince[1]
machmv5: mov bh, byte ptr [eax+88888888h] ;palookupoffs[1]

	mov eax, ecx
machmv13: shr eax, 32
machmv3: add ecx, 88888888h ;vince[0]
machmv1: mov al, byte ptr [eax+88888888h] ;bufplce[0]
	cmp al, 255
	adc dl, dl
machmv2: mov bl, byte ptr [eax+88888888h] ;palookupoffs[0]

	shl dl, 4
	xor eax, eax
fixchain2mb: add edi, 320
	mov al, dl
	add eax, offset mvcase0
	jmp eax       ;16 byte cases

ALIGN 16
endmvlineasm4:
	dec byte ptr _asm3[0]
	jnz beginmvlineasm42

	mov dword ptr _vplce[0], ecx
	mov dword ptr _vplce[4], edx
	mov dword ptr _vplce[8], esi
	mov dword ptr _vplce[12], ebp
	pop ebp
	ret

	;5,7,8,8,11,13,12,14,11,13,14,14,12,14,15,7
ALIGN 16
mvcase0:
	jmp beginmvlineasm4
ALIGN 16
mvcase1:
	mov byte ptr [edi], bl
	jmp beginmvlineasm4
ALIGN 16
mvcase2:
	mov byte ptr [edi+1], bh
	jmp beginmvlineasm4
ALIGN 16
mvcase3:
	mov word ptr [edi], bx
	jmp beginmvlineasm4
ALIGN 16
mvcase4:
	shr ebx, 16
	mov byte ptr [edi+2], bl
	jmp beginmvlineasm4
ALIGN 16
mvcase5:
	mov byte ptr [edi], bl
	shr ebx, 16
	mov byte ptr [edi+2], bl
	jmp beginmvlineasm4
ALIGN 16
	mvcase6:
	shr ebx, 8
	mov word ptr [edi+1], bx
	jmp beginmvlineasm4
ALIGN 16
mvcase7:
	mov word ptr [edi], bx
	shr ebx, 16
	mov byte ptr [edi+2], bl
	jmp beginmvlineasm4
ALIGN 16
mvcase8:
	shr ebx, 16
	mov byte ptr [edi+3], bh
	jmp beginmvlineasm4
ALIGN 16
mvcase9:
	mov byte ptr [edi], bl
	shr ebx, 16
	mov byte ptr [edi+3], bh
	jmp beginmvlineasm4
ALIGN 16
mvcase10:
	mov byte ptr [edi+1], bh
	shr ebx, 16
	mov byte ptr [edi+3], bh
	jmp beginmvlineasm4
ALIGN 16
mvcase11:
	mov word ptr [edi], bx
	shr ebx, 16
	mov byte ptr [edi+3], bh
	jmp beginmvlineasm4
ALIGN 16
mvcase12:
	shr ebx, 16
	mov word ptr [edi+2], bx
	jmp beginmvlineasm4
ALIGN 16
mvcase13:
	mov byte ptr [edi], bl
	shr ebx, 16
	mov word ptr [edi+2], bx
	jmp beginmvlineasm4
ALIGN 16
mvcase14:
	mov byte ptr [edi+1], bh
	shr ebx, 16
	mov word ptr [edi+2], bx
	jmp beginmvlineasm4
ALIGN 16
mvcase15:
	mov dword ptr [edi], ebx
	jmp beginmvlineasm4
	
ALIGN 16
PUBLIC setupspritevline_
setupspritevline_:
	mov dword ptr [spal+2], eax

	mov eax, esi                      ;xinc's
	shl eax, 16
	mov dword ptr [smach1+2], eax
	mov dword ptr [smach4+2], eax
	mov eax, esi
	sar eax, 16
	add eax, ebx                      ;watch out with ebx - it's passed
	mov dword ptr [smach2+2], eax
	add eax, edx
	mov dword ptr [smach5+2], eax

	mov dword ptr [smach3+2], ecx  ;yinc's
	ret

ALIGN 16
PUBLIC spritevline_
	;eax = 0, ebx = x, ecx = cnt, edx = y, esi = yplc, edi = p
prestartsvline:
smach1: add ebx, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
smach2: adc esi, 88888888h              ;xincshr16+yalwaysinc

startsvline:
spal: mov al, [eax+88888888h]           ;palookup
	mov byte ptr [edi], al
fixchain1s: add edi, 320

spritevline_:
smach3: add edx, 88888888h              ;dayinc
	dec ecx
	ja short prestartsvline     ;jump if (no carry (add)) and (not zero (dec))!
	jz short endsvline
smach4: add ebx, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
smach5: adc esi, 88888888h              ;xincshr16+yalwaysinc+daydime
	jmp short startsvline
endsvline:
	ret

ALIGN 16
PUBLIC msetupspritevline_
msetupspritevline_:
	mov dword ptr [mspal+2], eax

	mov eax, esi                      ;xinc's
	shl eax, 16
	mov dword ptr [msmach1+2], eax
	mov dword ptr [msmach4+2], eax
	mov eax, esi
	sar eax, 16
	add eax, ebx                      ;watch out with ebx - it's passed
	mov dword ptr [msmach2+2], eax
	add eax, edx
	mov dword ptr [msmach5+2], eax

	mov dword ptr [msmach3+2], ecx  ;yinc's
	ret

ALIGN 16
PUBLIC mspritevline_
	;eax = 0, ebx = x, ecx = cnt, edx = y, esi = yplc, edi = p
mprestartsvline:
msmach1: add ebx, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
msmach2: adc esi, 88888888h              ;xincshr16+yalwaysinc

mstartsvline:
	cmp al, 255
	je short mskipsvline
mspal: mov al, [eax+88888888h]           ;palookup
	mov byte ptr [edi], al
mskipsvline:
mfixchain1s: add edi, 320

mspritevline_:
msmach3: add edx, 88888888h              ;dayinc
	dec ecx
	ja short mprestartsvline     ;jump if (no carry (add)) and (not zero (dec))!
	jz short mendsvline
msmach4: add ebx, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
msmach5: adc esi, 88888888h              ;xincshr16+yalwaysinc+daydime
	jmp short mstartsvline
mendsvline:
	ret

ALIGN 16
PUBLIC tsetupspritevline_
tsetupspritevline_:
	mov dword ptr [tspal+2], eax

	mov eax, esi                      ;xinc's
	shl eax, 16
	mov dword ptr [tsmach1+2], eax
	mov dword ptr [tsmach4+2], eax
	mov eax, esi
	sar eax, 16
	add eax, ebx                      ;watch out with ebx - it's passed
	mov dword ptr [tsmach2+2], eax
	add eax, edx
	mov dword ptr [tsmach5+2], eax

	mov dword ptr [tsmach3+2], ecx  ;yinc's
	ret

ALIGN 16
PUBLIC tspritevline_
tspritevline_:
	;eax = 0, ebx = x, ecx = cnt, edx = y, esi = yplc, edi = p
	push ebp
	mov ebp, ebx
	xor ebx, ebx
	jmp tenterspritevline
ALIGN 16
tprestartsvline:
tsmach1: add ebp, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
tsmach2: adc esi, 88888888h              ;xincshr16+yalwaysinc

tstartsvline:
	cmp al, 255
	je short tskipsvline
transrev2:
	mov bh, byte ptr [edi]
transrev3:
tspal: mov bl, [eax+88888888h]               ;palookup
tmach4: mov al, byte ptr [ebx+88888888h]     ;_transluc
	mov byte ptr [edi], al
tskipsvline:
tfixchain1s: add edi, 320

tenterspritevline:
tsmach3: add edx, 88888888h              ;dayinc
	dec ecx
	ja short tprestartsvline     ;jump if (no carry (add)) and (not zero (dec))!
	jz short tendsvline
tsmach4: add ebp, 88888888h              ;xincshl16
	mov al, byte ptr [esi]
tsmach5: adc esi, 88888888h              ;xincshr16+yalwaysinc+daydime
	jmp short tstartsvline
tendsvline:
	pop ebp
	ret

ALIGN 16
PUBLIC msethlineshift_
msethlineshift_:
	neg al
	mov byte ptr [msh1d+2], al
	mov byte ptr [msh2d+3], bl
	mov byte ptr [msh3d+2], al
	mov byte ptr [msh4d+3], bl
	mov byte ptr [msh5d+2], al
	mov byte ptr [msh6d+3], bl
	ret

ALIGN 16
PUBLIC mhline_
mhline_:
	;_asm1 = bxinc
	;_asm2 = byinc
	;_asm3 = shadeoffs
	;eax = picoffs
	;ebx = bx
	;ecx = cnt
	;edx = ?
	;esi = by
	;edi = p

	mov dword ptr [mmach1d+2], eax
	mov dword ptr [mmach5d+2], eax
	mov dword ptr [mmach9d+2], eax
	mov eax, _asm3
	mov dword ptr [mmach2d+2], eax
	mov dword ptr [mmach2da+2], eax
	mov dword ptr [mmach2db+2], eax
	mov dword ptr [mmach6d+2], eax
	mov dword ptr [mmach10d+2], eax
	mov eax, _asm1
	mov dword ptr [mmach3d+2], eax
	mov dword ptr [mmach7d+2], eax
	mov eax, _asm2
	mov dword ptr [mmach4d+2], eax
	mov dword ptr [mmach8d+2], eax
	jmp short mhlineskipmodify_

ALIGN 16
PUBLIC mhlineskipmodify_
mhlineskipmodify_:

	push ebp

	xor eax, eax
	mov ebp, ebx

	test ecx, 00010000h
	jnz short mbeghline

msh1d: shr ebx, 26
msh2d: shld ebx, esi, 6
	add ebp, _asm1
mmach9d: mov al, byte ptr [ebx+88888888h]    ;picoffs
	add esi, _asm2
	cmp al, 255
	je mskip5

	mmach10d: mov cl, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi], cl
mskip5:
	inc edi
	sub ecx, 65536
	jc mendhline
	jmp short mbeghline

ALIGN 16
mpreprebeghline:                                            ;1st only
	mov al, cl
mmach2d: mov al, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi], al

mprebeghline:
	add edi, 2
	sub ecx, 131072
	jc short mendhline
mbeghline:
mmach3d: lea ebx, [ebp+88888888h]            ;bxinc
msh3d: shr ebp, 26
msh4d: shld ebp, esi, 6
mmach4d: add esi, 88888888h                  ;byinc
mmach1d: mov cl, byte ptr [ebp+88888888h]    ;picoffs
mmach7d: lea ebp, [ebx+88888888h]            ;bxinc

msh5d: shr ebx, 26
msh6d: shld ebx, esi, 6
mmach8d: add esi, 88888888h                  ;byinc
mmach5d: mov ch, byte ptr [ebx+88888888h]    ;picoffs

	cmp cl, 255
	je short mskip1
	cmp ch, 255
	je short mpreprebeghline

	mov al, cl                                                  ;BOTH
mmach2da: mov bl, byte ptr [eax+88888888h]    ;shadeoffs
	mov al, ch
mmach2db: mov bh, byte ptr [eax+88888888h]    ;shadeoffs
	mov word ptr [edi], bx
	add edi, 2
	sub ecx, 131072
	jnc short mbeghline
	jmp mendhline
mskip1:                                                     ;2nd only
	cmp ch, 255
	je short mprebeghline

	mov al, ch
mmach6d: mov al, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi+1], al
	add edi, 2
	sub ecx, 131072
	jnc short mbeghline
mendhline:

	pop ebp
	ret

ALIGN 16
PUBLIC tsethlineshift_
tsethlineshift_:
	neg al
	mov byte ptr [tsh1d+2], al
	mov byte ptr [tsh2d+3], bl
	mov byte ptr [tsh3d+2], al
	mov byte ptr [tsh4d+3], bl
	mov byte ptr [tsh5d+2], al
	mov byte ptr [tsh6d+3], bl
	ret

ALIGN 16
PUBLIC thline_
thline_:
	;_asm1 = bxinc
	;_asm2 = byinc
	;_asm3 = shadeoffs
	;eax = picoffs
	;ebx = bx
	;ecx = cnt
	;edx = ?
	;esi = by
	;edi = p

	mov dword ptr [tmach1d+2], eax
	mov dword ptr [tmach5d+2], eax
	mov dword ptr [tmach9d+2], eax
	mov eax, _asm3
	mov dword ptr [tmach2d+2], eax
	mov dword ptr [tmach6d+2], eax
	mov dword ptr [tmach10d+2], eax
	mov eax, _asm1
	mov dword ptr [tmach3d+2], eax
	mov dword ptr [tmach7d+2], eax
	mov eax, _asm2
	mov dword ptr [tmach4d+2], eax
	mov dword ptr [tmach8d+2], eax
	jmp thlineskipmodify_

ALIGN 16
PUBLIC thlineskipmodify_
thlineskipmodify_:

	push ebp

	xor eax, eax
	xor edx, edx
	mov ebp, ebx

	test ecx, 00010000h
	jnz short tbeghline

tsh1d: shr ebx, 26
tsh2d: shld ebx, esi, 6
	add ebp, _asm1
tmach9d: mov al, byte ptr [ebx+88888888h]    ;picoffs
	add esi, _asm2
	cmp al, 255
	je tskip5

transrev4:
tmach10d: mov dl, byte ptr [eax+88888888h]   ;shadeoffs
transrev5:
	mov dh, byte ptr [edi]
tmach1: mov al, byte ptr [edx+88888888h]     ;_transluc
	mov byte ptr [edi], al
tskip5:
	inc edi
	sub ecx, 65536
	jc tendhline
	jmp short tbeghline

ALIGN 16
tprebeghline:
	add edi, 2
	sub ecx, 131072
	jc short tendhline
tbeghline:
tmach3d: lea ebx, [ebp+88888888h]            ;bxinc
tsh3d: shr ebp, 26
tsh4d: shld ebp, esi, 6
tmach4d: add esi, 88888888h                  ;byinc
tmach1d: mov cl, byte ptr [ebp+88888888h]    ;picoffs
tmach7d: lea ebp, [ebx+88888888h]            ;bxinc

tsh5d: shr ebx, 26
tsh6d: shld ebx, esi, 6
tmach8d: add esi, 88888888h                  ;byinc
tmach5d: mov ch, byte ptr [ebx+88888888h]    ;picoffs

	cmp cx, 0ffffh
	je short tprebeghline

	mov bx, word ptr [edi]

	cmp cl, 255
	je short tskip1
	mov al, cl
transrev6:
tmach2d: mov dl, byte ptr [eax+88888888h]    ;shadeoffs
transrev7:
	mov dh, bl
tmach2: mov al, byte ptr [edx+88888888h]     ;_transluc
	mov byte ptr [edi], al

	cmp ch, 255
	je short tskip2
tskip1:
	mov al, ch
transrev8:
tmach6d: mov dl, byte ptr [eax+88888888h]    ;shadeoffs
transrev9:
	mov dh, bh
tmach3: mov al, byte ptr [edx+88888888h]     ;_transluc
	mov byte ptr [edi+1], al
tskip2:

	add edi, 2
	sub ecx, 131072
	jnc tbeghline
tendhline:

	pop ebp
	ret


	;eax=shiftval, ebx=palookup1, ecx=palookup2
ALIGN 16
PUBLIC setuptvlineasm2_
setuptvlineasm2_:
	mov byte ptr [tran2shra+2], al
	mov byte ptr [tran2shrb+2], al
	mov dword ptr [tran2pala+2], ebx
	mov dword ptr [tran2palb+2], ecx
	mov dword ptr [tran2palc+2], ebx
	mov dword ptr [tran2pald+2], ecx
	ret

	;Pass:   eax=vplc2, ebx=vinc1, ecx=bufplc1, edx=bufplc2, esi=vplc1, edi=p
	;        _asm1=vinc2, _asm2=pend
	;Return: _asm1=vplc1, _asm2=vplc2
ALIGN 16
PUBLIC tvlineasm2_
tvlineasm2_:
	push ebp

	mov ebp, eax

	mov dword ptr [tran2inca+2], ebx
	mov eax, _asm1
	mov dword ptr [tran2incb+2], eax

	mov dword ptr [tran2bufa+2], ecx         ;bufplc1
	mov dword ptr [tran2bufb+2], edx         ;bufplc2

	mov eax, _asm2
	sub edi, eax
	mov dword ptr [tran2edia+3], eax
	mov dword ptr [tran2edic+2], eax
	inc eax
	mov dword ptr [tran2edie+2], eax
fixchaint2a: sub eax, 320
	mov dword ptr [tran2edif+2], eax
	dec eax
	mov dword ptr [tran2edib+3], eax
	mov dword ptr [tran2edid+2], eax

	xor ecx, ecx
	xor edx, edx
	jmp short begintvline2

		;eax 0000000000  temp  temp
		;ebx 0000000000 odat2 odat1
		;ecx 0000000000000000 ndat1
		;edx 0000000000000000 ndat2
		;esi          vplc1
		;edi videoplc--------------
		;ebp          vplc2

ALIGN 16
		;LEFT ONLY
skipdraw2:
transrev10:
tran2edic: mov ah, byte ptr [edi+88888888h]      ;getpixel
transrev11:
tran2palc: mov al, byte ptr [ecx+88888888h]      ;palookup1
fixchaint2d: add edi, 320
tran2trac: mov bl, byte ptr [eax+88888888h]      ;_transluc
tran2edid: mov byte ptr [edi+88888888h-320], bl  ;drawpixel
	jnc short begintvline2
	jmp endtvline2

skipdraw1:
	cmp dl, 255
	jne short skipdraw3
fixchaint2b: add edi, 320
	jc short endtvline2

begintvline2:
	mov eax, esi
tran2shra: shr eax, 88h                      ;globalshift
	mov ebx, ebp
tran2shrb: shr ebx, 88h                      ;globalshift
tran2inca: add esi, 88888888h                ;vinc1
tran2incb: add ebp, 88888888h                ;vinc2
tran2bufa: mov cl, byte ptr [eax+88888888h]  ;bufplc1
	cmp cl, 255
tran2bufb: mov dl, byte ptr [ebx+88888888h]  ;bufplc2
	je short skipdraw1
	cmp dl, 255
	je short skipdraw2

	;mov ax        The transluscent reverse of both!
	;mov bl, ah
	;mov ah
	;mov bh

		;BOTH
transrev12:
tran2edia: mov bx, word ptr [edi+88888888h]      ;getpixels
transrev13:
	mov ah, bl
transrev14:
tran2pala: mov al, byte ptr [ecx+88888888h]      ;palookup1
transrev15:
tran2palb: mov bl, byte ptr [edx+88888888h]      ;palookup2
fixchaint2c: add edi, 320
tran2traa: mov al, byte ptr [eax+88888888h]      ;_transluc
tran2trab: mov ah, byte ptr [ebx+88888888h]      ;_transluc
tran2edib: mov word ptr [edi+88888888h-320], ax  ;drawpixels
	jnc short begintvline2
	jmp short endtvline2

	;RIGHT ONLY
skipdraw3:
transrev16:
tran2edie: mov ah, byte ptr [edi+88888889h]      ;getpixel
transrev17:
tran2pald: mov al, byte ptr [edx+88888888h]      ;palookup2
fixchaint2e: add edi, 320
tran2trad: mov bl, byte ptr [eax+88888888h]      ;_transluc
tran2edif: mov byte ptr [edi+88888889h-320], bl  ;drawpixel
	jnc short begintvline2

endtvline2:
	mov _asm1, esi
	mov _asm2, ebp

	pop ebp
	ret


BITSOFPRECISION equ 3
BITSOFPRECISIONPOW equ 8

;Double-texture mapping with palette lookup
;eax:  ylo1------------|----dat|----dat
;ebx:  ylo2--------------------|----cnt
;ecx:  000000000000000000000000|---temp
;edx:  xhi1-xlo1---------------|---yhi1
;esi:  xhi2-xlo2---------------|---yhi2
;edi:  ------------------------videopos
;ebp:  ----------------------------temp

ALIGN 16
PUBLIC setupslopevlin2_
setupslopevlin2_:
	mov dword ptr [slop3+2], edx ;ptr
	mov dword ptr [slop7+2], edx ;ptr
	mov dword ptr [slop4+2], esi ;tptr
	mov dword ptr [slop8+2], esi ;tptr
	mov byte ptr [slop2+2], ah   ;ybits
	mov byte ptr [slop6+2], ah   ;ybits
	mov dword ptr [slop9+2], edi ;pinc

	mov edx, 1
	mov cl, al
	add cl, ah
	shl edx, cl
	dec edx
	mov cl, ah
	ror edx, cl

	mov dword ptr [slop1+2], edx   ;ybits...xbits
	mov dword ptr [slop5+2], edx   ;ybits...xbits

	ret

ALIGN 16
PUBLIC slopevlin2_
slopevlin2_:
	push ebp
	xor ecx, ecx

slopevlin2begin:
	mov ebp, edx
slop1: and ebp, 88000088h                ;ybits...xbits
slop2: rol ebp, 6                        ;ybits
	add eax, _asm1                        ;xinc1<<xbits
	adc edx, _asm2                        ;(yinc1&0xffffff00)+(xinc1>>(32-xbits))
slop3: mov cl, byte ptr [ebp+88888888h]  ;bufplc

	mov ebp, esi
slop4: mov al, byte ptr [ecx+88888888h]  ;paloffs
slop5: and ebp, 88000088h                ;ybits...xbits
slop6: rol ebp, 6                        ;ybits
	add ebx, _asm3                        ;xinc2<<xbits
slop7: mov cl, byte ptr [ebp+88888888h]  ;bufplc
	adc esi, _asm4                        ;(yinc2&0xffffff00)+(xinc2>>(32-xbits))
slop8: mov ah, byte ptr [ecx+88888888h]  ;paloffs

	dec bl
	mov word ptr [edi], ax
slop9: lea edi, [edi+88888888h]          ;pinc
	jnz short slopevlin2begin

	pop ebp
	mov eax, edi
	ret


ALIGN 16
PUBLIC setupslopevlin_
setupslopevlin_:
	mov dword ptr [slopmach3+3], ebx    ;ptr
	mov dword ptr [slopmach5+2], ecx    ;pinc
	neg ecx
	mov dword ptr [slopmach6+2], ecx    ;-pinc

	mov edx, 1
	mov cl, al
	shl edx, cl
	dec edx
	mov cl, ah
	shl edx, cl
	mov dword ptr [slopmach7+2], edx

	neg ah
	mov byte ptr [slopmach2+2], ah

	sub ah, al
	mov byte ptr [slopmach1+2], ah

	fild dword ptr _asm1
	fstp dword ptr _asm2
	ret

ALIGN 16
PUBLIC slopevlin_
slopevlin_:
	mov _ebpbak, ebp
	mov _espbak, esp

	sub ecx, esp
	mov dword ptr [slopmach4+3], ecx

	fild dword ptr _asm3
slopmach6: lea ebp, [eax+88888888h]
	fadd dword ptr _asm2

	mov _asm1, ebx
	shl ebx, 3

	mov eax, _globalx3
	mov ecx, _globaly3
	imul eax, ebx
	imul ecx, ebx
	add esi, eax
	add edi, ecx

	mov ebx, edx
	jmp short bigslopeloop
ALIGN 16
bigslopeloop:
	fst dword ptr _fpuasm

	mov eax, _fpuasm
	add eax, eax
	sbb edx, edx
	mov ecx, eax
	shr ecx, 24
	and eax, 00ffe000h
	shr eax, 11
	sub cl, 2
	mov eax, dword ptr _reciptable[eax]
	shr eax, cl
	xor eax, edx
	mov edx, _asm1
	mov ecx, _globalx3
	mov _asm1, eax
	sub eax, edx
	mov edx, _globaly3
	imul ecx, eax
	imul eax, edx

	fadd dword ptr _asm2

	cmp ebx, BITSOFPRECISIONPOW
	mov _asm4, ebx
	mov cl, bl
	jl short slopeskipmin
	mov cl, BITSOFPRECISIONPOW
slopeskipmin:

;eax: yinc.............
;ebx:   0   0   0   ?
;ecx: xinc......... cnt
;edx:         ?
;esi: xplc.............
;edi: yplc.............
;ebp:     videopos

	mov ebx, esi
	mov edx, edi

beginnerslopeloop:
slopmach1: shr ebx, 20
	add esi, ecx
slopmach2: shr edx, 26
slopmach7: and ebx, 88888888h
	add edi, eax
slopmach5: add ebp, 88888888h                   ;pinc
slopmach3: mov dl, byte ptr [ebx+edx+88888888h] ;ptr
slopmach4: mov ebx, dword ptr [esp+88888888h]
	sub esp, 4
	dec cl
	mov al, byte ptr [ebx+edx] ;tptr
	mov ebx, esi
	mov [ebp], al
	mov edx, edi
	jnz short beginnerslopeloop

	mov ebx, _asm4
	sub ebx, BITSOFPRECISIONPOW
	jg bigslopeloop

	ffree st(0)

	mov esp, _espbak
	mov ebp, _ebpbak
	ret


ALIGN 16
PUBLIC setuprhlineasm4_
setuprhlineasm4_:
	mov dword ptr [rmach1a+2], eax
	mov dword ptr [rmach1b+2], eax
	mov dword ptr [rmach1c+2], eax
	mov dword ptr [rmach1d+2], eax
	mov dword ptr [rmach1e+2], eax

	mov dword ptr [rmach2a+2], ebx
	mov dword ptr [rmach2b+2], ebx
	mov dword ptr [rmach2c+2], ebx
	mov dword ptr [rmach2d+2], ebx
	mov dword ptr [rmach2e+2], ebx

	mov dword ptr [rmach3a+2], ecx
	mov dword ptr [rmach3b+2], ecx
	mov dword ptr [rmach3c+2], ecx
	mov dword ptr [rmach3d+2], ecx
	mov dword ptr [rmach3e+2], ecx

	mov dword ptr [rmach4a+2], edx
	mov dword ptr [rmach4b+2], edx
	mov dword ptr [rmach4c+2], edx
	mov dword ptr [rmach4d+2], edx
	mov dword ptr [rmach4e+2], edx

	mov dword ptr [rmach5a+2], esi
	mov dword ptr [rmach5b+2], esi
	mov dword ptr [rmach5c+2], esi
	mov dword ptr [rmach5d+2], esi
	mov dword ptr [rmach5e+2], esi
	ret

	;Non power of 2, non masking, with palookup method #1 (6 clock cycles)
	;eax: dat dat dat dat
	;ebx:          bufplc
	;ecx:  0          dat
	;edx:  xlo
	;esi:  ylo
	;edi:  videopos/cnt
	;ebp:  tempvar
	;esp:
ALIGN 16
PUBLIC rhlineasm4_
rhlineasm4_:
	push ebp

	cmp eax, 0
	jle endrhline

	lea ebp, [edi-4]
	sub ebp, eax
	mov dword ptr [rmach6a+2], ebp
	add ebp, 3
	mov dword ptr [rmach6b+2], ebp
	mov edi, eax
	test edi, 3
	jz short begrhline
	jmp short startrhline1

ALIGN 16
startrhline1:
	mov cl, byte ptr [ebx]                      ;bufplc
rmach1e: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmach2e: sub esi, 88888888h                    ;ylo
rmach3e: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmach4e: mov al, byte ptr [ecx+88888888h]      ;palookup
rmach5e: and ebp, 88888888h                    ;tilesizy
rmach6b: mov byte ptr [edi+88888888h], al      ;vidcntoffs
	sub ebx, ebp
	dec edi
	test edi, 3
	jnz short startrhline1
	test edi, edi
	jz endrhline

begrhline:
	mov cl, byte ptr [ebx]                      ;bufplc
rmach1a: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmach2a: sub esi, 88888888h                    ;ylo
rmach3a: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmach5a: and ebp, 88888888h                    ;tilesizy
	sub ebx, ebp

rmach1b: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmach4a: mov ah, byte ptr [ecx+88888888h]      ;palookup
	mov cl, byte ptr [ebx]                      ;bufplc
rmach2b: sub esi, 88888888h                    ;ylo
rmach3b: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmach5b: and ebp, 88888888h                    ;tilesizy
rmach4b: mov al, byte ptr [ecx+88888888h]      ;palookup
	sub ebx, ebp

	shl eax, 16

	mov cl, byte ptr [ebx]                      ;bufplc
rmach1c: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmach2c: sub esi, 88888888h                    ;ylo
rmach3c: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmach5c: and ebp, 88888888h                    ;tilesizy
	sub ebx, ebp

rmach1d: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmach4c: mov ah, byte ptr [ecx+88888888h]      ;palookup
	mov cl, byte ptr [ebx]                      ;bufplc
rmach2d: sub esi, 88888888h                    ;ylo
rmach3d: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmach5d: and ebp, 88888888h                    ;tilesizy
rmach4d: mov al, byte ptr [ecx+88888888h]      ;palookup
	sub ebx, ebp

rmach6a: mov dword ptr [edi+88888888h], eax    ;vidcntoffs
	sub edi, 4
	jnz begrhline
endrhline:
	pop ebp
	ret

ALIGN 16
PUBLIC setuprmhlineasm4_
setuprmhlineasm4_:
	mov dword ptr [rmmach1+2], eax
	mov dword ptr [rmmach2+2], ebx
	mov dword ptr [rmmach3+2], ecx
	mov dword ptr [rmmach4+2], edx
	mov dword ptr [rmmach5+2], esi
	ret

ALIGN 16
PUBLIC rmhlineasm4_
rmhlineasm4_:
	push ebp

	cmp eax, 0
	jle short endrmhline

	lea ebp, [edi-1]
	sub ebp, eax
	mov dword ptr [rmmach6+2], ebp
	mov edi, eax
	jmp short begrmhline

ALIGN 16
begrmhline:
	mov cl, byte ptr [ebx]                      ;bufplc
rmmach1: sub edx, 88888888h                    ;xlo
	sbb ebp, ebp
rmmach2: sub esi, 88888888h                    ;ylo
rmmach3: sbb ebx, 88888888h                    ;xhi*tilesizy + yhi+ycarry
rmmach5: and ebp, 88888888h                    ;tilesizy
	cmp cl, 255
	je short rmskip
rmmach4: mov al, byte ptr [ecx+88888888h]      ;palookup
rmmach6: mov byte ptr [edi+88888888h], al      ;vidcntoffs
rmskip:
	sub ebx, ebp
	dec edi
	jnz short begrmhline
endrmhline:
	pop ebp
	ret

ALIGN 16
PUBLIC setupqrhlineasm4_
setupqrhlineasm4_:
	mov dword ptr [qrmach2e+2], ebx
	mov dword ptr [qrmach3e+2], ecx
	xor edi, edi
	sub edi, ecx
	mov dword ptr [qrmach7a+2], edi
	mov dword ptr [qrmach7b+2], edi

	add ebx, ebx
	adc ecx, ecx
	mov dword ptr [qrmach2a+2], ebx
	mov dword ptr [qrmach2b+2], ebx
	mov dword ptr [qrmach3a+2], ecx
	mov dword ptr [qrmach3b+2], ecx

	mov dword ptr [qrmach4a+2], edx
	mov dword ptr [qrmach4b+2], edx
	mov dword ptr [qrmach4c+2], edx
	mov dword ptr [qrmach4d+2], edx
	mov dword ptr [qrmach4e+2], edx
	ret

	;Non power of 2, non masking, with palookup method (FASTER BUT NO SBB'S)
	;eax: dat dat dat dat
	;ebx:          bufplc
	;ecx:  0          dat
	;edx:  0          dat
	;esi:  ylo
	;edi:  videopos/cnt
	;ebp:  ?
	;esp:
ALIGN 16
PUBLIC qrhlineasm4_          ;4 pixels in 9 cycles!  2.25 cycles/pixel
qrhlineasm4_:
	push ebp

	cmp eax, 0
	jle endqrhline

	mov ebp, eax
	test ebp, 3
	jz short skipqrhline1
	jmp short startqrhline1

ALIGN 16
startqrhline1:
	mov cl, byte ptr [ebx]                      ;bufplc
	dec edi
qrmach2e: sub esi, 88888888h                   ;ylo
	dec ebp
qrmach3e: sbb ebx, 88888888h                   ;xhi*tilesizy + yhi+ycarry
qrmach4e: mov al, byte ptr [ecx+88888888h]     ;palookup
	mov byte ptr [edi], al                      ;vidcntoffs
	test ebp, 3
	jnz short startqrhline1
	test ebp, ebp
	jz short endqrhline

skipqrhline1:
	mov cl, byte ptr [ebx]                      ;bufplc
	jmp short begqrhline
ALIGN 16
begqrhline:
qrmach7a: mov dl, byte ptr [ebx+88888888h]     ;bufplc
qrmach2a: sub esi, 88888888h                   ;ylo
qrmach3a: sbb ebx, 88888888h                   ;xhi*tilesizy + yhi+ycarry
qrmach4a: mov ah, byte ptr [ecx+88888888h]     ;palookup
qrmach4b: mov al, byte ptr [edx+88888888h]     ;palookup
	sub edi, 4
	shl eax, 16
	mov cl, byte ptr [ebx]                      ;bufplc
qrmach7b: mov dl, byte ptr [ebx+88888888h]     ;bufplc
qrmach2b: sub esi, 88888888h                   ;ylo
qrmach3b: sbb ebx, 88888888h                   ;xhi*tilesizy + yhi+ycarry
qrmach4c: mov ah, byte ptr [ecx+88888888h]     ;palookup
qrmach4d: mov al, byte ptr [edx+88888888h]     ;palookup
	mov cl, byte ptr [ebx]                      ;bufplc
	mov dword ptr [edi], eax
	sub ebp, 4
	jnz short begqrhline

endqrhline:
	pop ebp
	ret


PUBLIC setupdrawslab_
setupdrawslab_:
	mov dword ptr [voxbpl1+2], eax
	mov dword ptr [voxbpl2+2], eax
	mov dword ptr [voxbpl3+2], eax
	mov dword ptr [voxbpl4+2], eax
	mov dword ptr [voxbpl5+2], eax
	mov dword ptr [voxbpl6+2], eax
	mov dword ptr [voxbpl7+2], eax
	mov dword ptr [voxbpl8+2], eax

	mov dword ptr [voxpal1+2], ebx
	mov dword ptr [voxpal2+2], ebx
	mov dword ptr [voxpal3+2], ebx
	mov dword ptr [voxpal4+2], ebx
	mov dword ptr [voxpal5+2], ebx
	mov dword ptr [voxpal6+2], ebx
	mov dword ptr [voxpal7+2], ebx
	mov dword ptr [voxpal8+2], ebx
	ret

ALIGN 16
PUBLIC drawslab_
drawslab_:
	push ebp
	cmp eax, 2
	je voxbegdraw2
	ja voxskip2
	xor eax, eax
voxbegdraw1:
	mov ebp, ebx
	shr ebp, 16
	add ebx, edx
	dec ecx
	mov al, byte ptr [esi+ebp]
voxpal1: mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi], al
voxbpl1: lea edi, [edi+88888888h]
	jnz voxbegdraw1
	pop ebp
	ret

voxbegdraw2:
	mov ebp, ebx
	shr ebp, 16
	add ebx, edx
	xor eax, eax
	dec ecx
	mov al, byte ptr [esi+ebp]
voxpal2: mov al, byte ptr [eax+88888888h]
	mov ah, al
	mov word ptr [edi], ax
voxbpl2: lea edi, [edi+88888888h]
	jnz voxbegdraw2
	pop ebp
	ret

voxskip2:
	cmp eax, 4
	jne voxskip4
	xor eax, eax
voxbegdraw4:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal3: mov al, byte ptr [eax+88888888h]
	mov ah, al
	shl eax, 8
	mov al, ah
	shl eax, 8
	mov al, ah
	mov dword ptr [edi], eax
voxbpl3: add edi, 88888888h
	dec ecx
	jnz voxbegdraw4
	pop ebp
	ret

voxskip4:
	add eax, edi

	test edi, 1
	jz voxskipslab1
	cmp edi, eax
	je voxskipslab1

	push eax
	push ebx
	push ecx
	push edi
voxbegslab1:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal4: mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi], al
voxbpl4: add edi, 88888888h
	dec ecx
	jnz voxbegslab1
	pop edi
	pop ecx
	pop ebx
	pop eax
	inc edi

voxskipslab1:
	push eax
	test edi, 2
	jz voxskipslab2
	dec eax
	cmp edi, eax
	jge voxskipslab2

	push ebx
	push ecx
	push edi
voxbegslab2:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal5: mov al, byte ptr [eax+88888888h]
	mov ah, al
	mov word ptr [edi], ax
voxbpl5: add edi, 88888888h
	dec ecx
	jnz voxbegslab2
	pop edi
	pop ecx
	pop ebx
	add edi, 2

voxskipslab2:
	mov eax, [esp]

	sub eax, 3
	cmp edi, eax
	jge voxskipslab3

voxprebegslab3:
	push ebx
	push ecx
	push edi
voxbegslab3:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal6: mov al, byte ptr [eax+88888888h]
	mov ah, al
	shl eax, 8
	mov al, ah
	shl eax, 8
	mov al, ah
	mov dword ptr [edi], eax
voxbpl6: add edi, 88888888h
	dec ecx
	jnz voxbegslab3
	pop edi
	pop ecx
	pop ebx
	add edi, 4

	mov eax, [esp]

	sub eax, 3
	cmp edi, eax
	jl voxprebegslab3

voxskipslab3:
	mov eax, [esp]

	dec eax
	cmp edi, eax
	jge voxskipslab4

	push ebx
	push ecx
	push edi
voxbegslab4:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal7: mov al, byte ptr [eax+88888888h]
	mov ah, al
	mov word ptr [edi], ax
voxbpl7: add edi, 88888888h
	dec ecx
	jnz voxbegslab4
	pop edi
	pop ecx
	pop ebx
	add edi, 2

voxskipslab4:
	pop eax

	cmp edi, eax
	je voxskipslab5

voxbegslab5:
	mov ebp, ebx
	add ebx, edx
	shr ebp, 16
	xor eax, eax
	mov al, byte ptr [esi+ebp]
voxpal8: mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi], al
voxbpl8: add edi, 88888888h
	dec ecx
	jnz voxbegslab5

voxskipslab5:
	pop ebp
	ret

;modify: loinc
;eax: |  dat   |  dat   |   dat  |   dat  |
;ebx: |      loplc1                       |
;ecx: |      loplc2     |  cnthi |  cntlo |
;edx: |--------|--------|--------| hiplc1 |
;esi: |--------|--------|--------| hiplc2 |
;edi: |--------|--------|--------| vidplc |
;ebp: |--------|--------|--------|  hiinc |

PUBLIC stretchhline_
stretchhline_:
	push ebp

	mov eax, ebx
	shl ebx, 16
	sar eax, 16
	and ecx, 0000ffffh
	or ecx, ebx

	add esi, eax
	mov eax, edx
	mov edx, esi

	mov ebp, eax
	shl eax, 16
	sar ebp, 16

	add ecx, eax
	adc esi, ebp

	add eax, eax
	adc ebp, ebp
	mov dword ptr [loinc1+2], eax
	mov dword ptr [loinc2+2], eax
	mov dword ptr [loinc3+2], eax
	mov dword ptr [loinc4+2], eax

	inc ch

	jmp begloop

begloop:
	mov al, [edx]
loinc1: sub ebx, 88888888h
	sbb edx, ebp
	mov ah, [esi]
loinc2: sub ecx, 88888888h
	sbb esi, ebp
	sub edi, 4
	shl eax, 16
loinc3: sub ebx, 88888888h
	mov al, [edx]
	sbb edx, ebp
	mov ah, [esi]
loinc4: sub ecx, 88888888h
	sbb esi, ebp
	mov [edi], eax
	dec cl
	jnz begloop
	dec ch
	jnz begloop

	pop ebp
	ret


PUBLIC mmxoverlay_
mmxoverlay_:
	pushfd                 ;Check if CPUID is available
	pop eax
	mov ebx, eax
	xor eax, 00200000h
	push eax
	popfd
	pushfd
	pop eax
	cmp eax, ebx
	je pentium
	xor eax, eax
	dw 0a20fh
	test eax, eax
	jz pentium
	mov eax, 1
	dw 0a20fh
	and eax, 00000f00h
	test edx, 00800000h    ;Check if MMX is available
	jz nommx
	cmp eax, 00000600h     ;Check if P6 Family or not
	jae pentiumii
	jmp pentiummmx
nommx:
	cmp eax, 00000600h     ;Check if P6 Family or not
	jae pentiumpro
pentium:
	ret

;+--------------------------------------------------------------+
;|                    PENTIUM II Overlays                       |
;+--------------------------------------------------------------+
pentiumii:
		;Hline overlay (MMX doens't help)
	mov byte ptr [sethlinesizes_], 0xe9
	mov dword ptr [sethlinesizes_+1], (offset prosethlinesizes_)-(offset sethlinesizes_)-5
	mov byte ptr [setpalookupaddress_], 0xe9
	mov dword ptr [setpalookupaddress_+1], (offset prosetpalookupaddress_)-(offset setpalookupaddress_)-5
	mov byte ptr [setuphlineasm4_], 0xc3  ;ret (no code required)
	mov byte ptr [hlineasm4_], 0xe9
	mov dword ptr [hlineasm4_+1], (offset prohlineasm4_)-(offset hlineasm4_)-5

		;Vline overlay
	mov byte ptr [setupvlineasm_], 0xe9
	mov dword ptr [setupvlineasm_+1], (offset prosetupvlineasm_)-(offset setupvlineasm_)-5
	mov byte ptr [vlineasm4_], 0xe9
	mov dword ptr [vlineasm4_+1], (offset provlineasm4_)-(offset vlineasm4_)-5

	ret

;+--------------------------------------------------------------+
;|                    PENTIUM MMX Overlays                      |
;+--------------------------------------------------------------+
pentiummmx:
	ret

;+--------------------------------------------------------------+
;|                    PENTIUM PRO Overlays                      |
;+--------------------------------------------------------------+
pentiumpro:
		;Hline overlay (MMX doens't help)
	mov byte ptr [sethlinesizes_], 0xe9
	mov dword ptr [sethlinesizes_+1], (offset prosethlinesizes_)-(offset sethlinesizes_)-5
	mov byte ptr [setpalookupaddress_], 0xe9
	mov dword ptr [setpalookupaddress_+1], (offset prosetpalookupaddress_)-(offset setpalookupaddress_)-5
	mov byte ptr [setuphlineasm4_], 0xc3  ;ret (no code required)
	mov byte ptr [hlineasm4_], 0xe9
	mov dword ptr [hlineasm4_+1], (offset prohlineasm4_)-(offset hlineasm4_)-5

		;Vline overlay
	mov byte ptr [setupvlineasm_], 0xe9
	mov dword ptr [setupvlineasm_+1], (offset prosetupvlineasm_)-(offset setupvlineasm_)-5
	mov byte ptr [vlineasm4_], 0xe9
	mov dword ptr [vlineasm4_+1], (offset provlineasm4_)-(offset vlineasm4_)-5

	ret

CODE ENDS
END
