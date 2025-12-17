        .area GSINIT3 (CODE)
__mcs51_genXINIT::
        mov r1,#l_XINIT
        mov a,r1
        orl a,#(l_XINIT >> 8)
        jz 00003$
        mov r2,#((l_XINIT+255) >> 8)
        mov dptr,#s_XINIT
        mov r0,#s_XISEG
        mov __XPAGE,#(s_XISEG >> 8)
00001$: clr a
        movc a,@a+dptr
        movx @r0,a
        inc dptr
        inc r0
        cjne r0,#0,00002$
        inc __XPAGE
00002$: djnz r1,00001$
        djnz r2,00001$
        mov __XPAGE,#0xFF
00003$:

