/**********************************************************************
 * @file break_points1.S
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 **********************************************************************
 * Breakpoint ASM coede
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

//TODO: 使用mfcr/mtcr cr<14, 1>访问普通用户sp寄存器
#include <csi_config.h>

.import Breakpoint_Handler_C


.text
    .align  2
    .global Breakpoint_Handler
    .type   Breakpoint_Handler, %function
Breakpoint_Handler:
    //把r15 PUSH到栈，使用push,pop指令操作r15无法二次进入中断
    subi    sp, 4
    stw     r0, (sp)

    //批量保存r1-r13
    lrw     r0, serdbg_regsave+4
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

    //保存epsr和epc
    mfcr    r1, epsr
    stw     r1, (r0, 64)
    mfcr    r1, epc
    stw     r1, (r0, 68)
    subi    r0, 128     //r0=serdbg_regsave

    //保存刚才栈里的r0
    ldw     r1, (sp)
    stw     r1, (r0)

    //r0已经是第一个参数所需的值了
    bsr     Breakpoint_Handler_C
    //在C语言中把断点处的代码搬到RAM

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

    rte     //epc和epsr已经设置过了
    .size   Breakpoint_Handler, . - Breakpoint_Handler
