/*
 * libstub.S
 */

	.text

	.globl	__aeabi_uidiv
__aeabi_uidiv:
	push	{lr}
	mov	r2, #0
	bl	__Divide
	pop	{pc}

	.globl	__aeabi_uidivmod
__aeabi_uidivmod:
	push	{lr}
	sub	sp, sp, #12
	add	r2, sp, #4
	bl	__Divide
	ldr	r1, [sp, #4]
	add	sp, sp, #12
	pop	{pc}

	.globl	__aeabi_idiv
__aeabi_idiv:
	b	__DivideInteger

	.globl	__aeabi_idivmod
__aeabi_idivmod:
	push {r0, r1, lr}
  bl __aeabi_idiv
  pop {r1, r2, lr}
  mul r3, r2, r0
  sub r1, r1, r3
  bx lr

/* End */
