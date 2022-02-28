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

.data
    .align  2
    .global RunInstRAM
    .type   RunInstRAM, %function
RunInstRAM:     //在内存中运行断点处的指令
    //填充内容
    asri16  r0, r0, 0  //被覆盖的16位内容
    asri16  r0, r0, 0  //16位空指令，用于扩展到32位

    //psr, 不屏蔽无法再次进入中断
    /*push    r15
    mfcr    r15, psr
    mtcr    r15, epsr
    pop     r15*/

    rte
    .size   RunInstRAM, . - RunInstRAM


.text
    .align  2
    .global Breakpoint_Handler
    .type   Breakpoint_Handler, %function
Breakpoint_Handler:
    //把r15 PUSH到栈，使用push,pop指令操作r15无法二次进入中断
    subi    sp, 4
    stw     r15, (sp)

    //保存r0-r13
    lrw     r15, serdbg_regsave
    stm     r0-r13, (r15)

    //保存r14
    mov     r0, sp
    addi    r0, 4
    stw     r0, (r15, 56)

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
    mfcr   r0, epsr
    stw    r0, (r15, 192)
    mfcr   r0, epc
    stw    r0, (r15, 196)

    mov    r0,  r15    //C函数的参数
    lrw    r15,  Breakpoint_Handler_C
    jsr    r15         //跳转到C函数
Load_Regs_Run:
//r0: 原来的16bit内容
    lrw     r2, RunInstRAM
    sth     r0, (r2)

    lrw     r15, serdbg_regsave
    cmphsi  r0, 0xc000      //判断是否为32为指令
    movi    r1, 0x5000      //先写空指令
    mfcr    r3, epc         //r3=epc
    addi    r3, 2           //r3+=2
    bf      ll16b           //不是32位指令，跳到后面

    //是32位指令
    ldh     r1, (r3, 2)     //加载Flash中的后半字
    addi    r3, 2
ll16b:
    sth     r1, (r2, 2)     //保存后半字到RAM程序
    mtcr    r3, epc

    ldm     r0-r13, (r15)   //暂时不恢复r14和r15
    addi    r15, 64
    ldm     r16-r31, (r15)
    addi    r15, 64
    fldms   vr0-vr15, (r15)
    subi    r15, 128

    //恢复psr，不屏蔽无法再次进入中断
    /*ldw     r15, (r15, 192)
    mtcr    r15, psr*/

    //恢复r15
    ldw     r15, (sp)
    addi    sp, 4

    jmpi    RunInstRAM  //跳转到内存中执行
    .size   Breakpoint_Handler, . - Breakpoint_Handler
