/**********************************************************************
 * Track Handler ASM part
 * Copyright (C) 2022  Xu Ruijun
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 **********************************************************************/
.text
    .align 2
    .global Track_Handler
    .type   Track_Handler, %function
Track_Handler:
    subi    sp, 4
    stw     r0, (sp)
    lrw     r0, serdbg_regsave+4

    //批量保存r1-r13
    stm     r1-r13, (r0)

    //保存中断前的r14(sp)
    mov     r1, sp
    addi    r1, 4
    stw     r1, (r0, 52)

    //批量保存r15-r31, vr0-vr15
    addi    r0, 56
    stm     r15-r31, (r0)
    addi    r0, 68
    fstms   vr0-vr15, (r0)

    //保存epsr
    mfcr    r1, epsr
    stw     r1, (r0, 64)
    subi    r0, 128    //r0=serdbg_regsave

    //转存刚才栈里的r0
    ldw     r1, (sp)
    stw     r1, (r0)

    //当前epc作为C语言函数的第二个参数，不直接保存
    mfcr    r1, epc

    //进入C语言函数
    bsr     Track_Handler_C

    //恢复epsr, epc
    lrw     r0, serdbg_regsave+128
    ldw     r1, (r0, 64)
    mtcr    r1, epsr
    ldw     r1, (r0, 68)
    mtcr    r1, epc

    //批量恢复vr0-vr15, r1-r31
    fldms   vr0-vr15, (r0)
    subi    r0, 124
    ldm     r1-r31, (r0)

    //恢复r0
    subi    r0, 4
    ldw     r0, (r0)
    rte
