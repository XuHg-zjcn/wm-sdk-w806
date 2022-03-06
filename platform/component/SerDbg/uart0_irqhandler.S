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
    nie         //epsr epc
    ipush       //r0-r3, r12, r13
    subi    sp, 4
    stw     r15, (sp)
    //sp :  +0  +4  +8 +12 +16 +20 +24  +28 +32 | +36 ...
    //reg: r15  r0  r1  r2  r3 r12 r13 epsr epc | old ...

    //获取状态
    lrw     r0, serdbg_regsave+200
    ldw     r1, (r0)

    //判断是否需要停下来
    cmpnei  r1, 0x01
    bt      Enter_AT_Parser

    //清除标志，下次不要再停下来了
    movi    r1, 0
    stw     r1, (r0)

    //转存r0
    subi    r0, 200
    ldw     r1, (sp, 4)
    stw     r1, (r0)

    //批量保存r1-r13
    addi    r0, 4
    ldw     r1, (sp, 8)
    stm     r1-r13, (r0)

    //保存中断前的r14(sp)值
    mov     r1, sp
    addi    r1, 36
    stw     r1, (r0, 52)

    //保存r15
    stw     r15, (r0, 56)

    //批量保存r16-r31, vr0-vr15
    addi    r0, 60
    stm     r16-r31, (r0)
    addi    r0, 64
    fstms   vr0-vr15, (r0)
    subi    r0, 128

    //保存epsr和epc
    ldw     r1, (sp, 28)
    stw     r1, (r0, 192)
    ldw     r1, (sp, 32)
    stw     r1, (r0, 196)
    bsr     serdbg_parser_cmd

    lrw     r0,  huart0
    bsr     atcmd_parser_IRQHandler  //TODO: 自己实现一个AT指令解析器

    lrw     r0, serdbg_regsave+128
    ldw     r1, (r0, 64)
    stw     r1, (sp, 28)  //写入epsr
    ldw     r1, (r0, 68)
    stw     r1, (sp, 32)  //写入epc

    //恢复vr0-vr15, r16-r31, r1-r13, r15
    fldms   vr0-vr15, (r0)
    subi    r0, 64
    ldm     r16-r31, (r0)
    subi    r0, 60
    ldm     r1-r13, (r0)

    ldw     r15, (sp)
    addi    sp, 4
    ipop
    nir

Enter_AT_Parser:
    lrw     r0,  huart0
    bsr     atcmd_parser_IRQHandler  //TODO: 自己实现一个AT指令解析器

    ldw     r15, (sp)
    addi    sp, 4
    ipop
    nir
