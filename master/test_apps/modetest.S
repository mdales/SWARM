
#include <asm.h>

CENTRY(start)
	mov r0, #0x13
	msr spsr, r0
	mov lr, #0
	movs pc, lr
	swi 0x00800001