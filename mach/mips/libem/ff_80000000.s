#
.sect .text; .sect .rom; .sect .data; .sect .bss

.sect .rom

/* 2147483648 as a float; used as a pivot for double->float and unsigned->float. */

.define .ff_80000000
.ff_80000000:
	.data4 0x4f000000

