	.globl __start__stack
;--------------------------------------------------------
; Stack segment in internal ram
;--------------------------------------------------------
	.area	SSEG	(DATA)
__start__stack:
	.ds	1

 	.area VECTOR    (CODE)
	.globl __interrupt_vect
__interrupt_vect:
 	ljmp	__sdcc_gsinit_startup
 	ljmp	_isr_ext0	; 0x03
	.ds     5
	ljmp	_isr_timer0	; 0x0b
	.ds     5
	ljmp	_isr_ext1	; 0x13
	.ds     5
	reti
	.ds     7
 	ljmp    _isr_serial	; 0x23
	.ds     5
	reti			; 0x2b TIMER 2 IRQ
	.ds     7
	reti			; 0x33 NOT used by DW8051
	.ds     7
	reti			; 0x3b Serial port 1 RX/TX IRQ
	.ds     7
 	ljmp    _isr_ext2	; 0x43
	.ds     5
 	ljmp    _isr_ext3	; 0x4b

	.globl __start__stack

	.area GSINIT0 (CODE)

__sdcc_gsinit_startup::
        mov     sp,#__start__stack - 1

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

	.area GSFINAL (CODE)
        ljmp	_bootloader

__sdcc_banked_call::
	push	_PSBANK
	xch	a,r0
	push	a
	mov	a,r1
	push	a
	mov	a,r2
	anl	a,#0x1f
	mov	_PSBANK, a
	xch	a, r0
	ret

__sdcc_banked_ret::
	pop	_PSBANK
	ret
