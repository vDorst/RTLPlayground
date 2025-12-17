        .area GSINIT4 (CODE)
__mcs51_genXRAMCLEAR::
        mov r0,#l_PSEG
        mov a,r0
        orl a,#(l_PSEG >> 8)
        jz 00006$
        mov r1,#s_PSEG
        mov __XPAGE,#(s_PSEG >> 8)
        clr a
00005$: movx @r1,a
        inc r1
        djnz r0,00005$
00006$:
        mov r0,#l_XSEG
        mov a,r0
        orl a,#(l_XSEG >> 8)
jz 00008$
        mov r1,#((l_XSEG + 255) >> 8)
        mov dptr,#s_XSEG
        clr a
00007$: movx @dptr,a
        inc dptr
        djnz r0,00007$
        djnz r1,00007$
00008$:
