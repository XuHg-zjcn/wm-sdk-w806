/**********************************************************************
 * 串口中断保存寄存器
 * Copyright (C) 2021-2022  Xu Ruijun
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
    .align  2
    .global UART0_IRQHandler
    .type   UART0_IRQHandler, %function
UART0_IRQHandler:
    nie
    subi      sp,  4
	stw       r15, (sp)

    //TODO: 用flag判断是否需要保存全部寄存器，节省中断耗时
    //保存r0-r13
    lrw     r15, serdbg_regsave
    stm     r0-r13, (r15)

    //保存r14(sp)
    mov    r13, sp
    subi   r13, 12
    stw    r13, (r15, 56)

    //保存刚才栈里的r15
    ldw    r13, (sp)
    stw    r13, (r15, 60)

    //保存r16-r31, vr0-vr15
    addi   r15, 64
    stm    r16-r31, (r15)
    addi   r15, 64
    fstms  vr0-vr15, (r15)
    subi   r15, 128

    //保存epsr和epc
    ldw    r0, (sp, 4)    //mfcr   r0, epsr
    stw    r0, (r15, 192)
    ldw    r0, (sp, 8)    //mfcr   r0, epc
    stw    r0, (r15, 196)

    lrw    r0,  huart0
    bsr    atcmd_parser_IRQHandler  //TODO: 自己实现一个AT指令解析器

    lrw    r15, serdbg_regsave
    ldm    r0-r13, (r15)   //暂时不恢复r14和r15
    addi   r15, 64
    ldm    r16-r31, (r15)
    addi   r15, 64
    fldms  vr0-vr15, (r15)
    subi   r15, 128

    ldw    r15, (sp)
    addi   sp,  4
    nir
