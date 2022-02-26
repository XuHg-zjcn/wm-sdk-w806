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

.section .bss
    .global bkpt_stack_base
    .global bkpt_stack_top
bkpt_stack_base:
    .space 4096  //stack
bkpt_stack_top:
    .space 200   //保存寄存器

.data
    .align  2
    .global RunInstRAM
    .type   RunInstRAM, %function
RunInstRAM:     //在内存中运行断点处的指令
    //填充内容
    .short 0           //被覆盖的16位内容
    lsli16  r0, r0, 0  //16位空指令，用于扩展到32位

    //保存r15
    subi    sp, 4
    stw     r15, (sp)

    //跳到下一个PC地址，请在调用本函数前+2或+4
    lrw     r15, bkpt_stack_top
    ldw     r15, (r15, 196)
    mtcr    r15, epc

    //当前PSR
    mfcr    r15, psr
    mtcr    r15, epsr

    //恢复r15和sp
    ldw     r15, (sp)
    addi    sp, 4
    rte
    .size   RunInstRAM, . - RunInstRAM


.text
    .align  2
    .global Breakpoint_Handler
    .type   Breakpoint_Handler, %function
Breakpoint_Handler:
    //把r15 PUSH到栈
    subi    sp, 4
    stw     r15, (sp)
    addi    sp, 4      //恢复原来栈指针，供下面保存

    //保存r0-r13
    lrw     r15, bkpt_stack_top
    stm     r0-r14, (r15)

    //保存普通用户r14(sp)
    //mfcr    r13, usp
    //stw     r13, (r15, 56)

    //保存刚才栈里的r15
    subi   sp, 4
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

    mov    sp,  r15    //切换到新栈
    mov    r0,  r15    //C函数的参数

    lrw    r15,  Breakpoint_Handler_C
    jsr    r15         //跳转到C函数
Load_Regs_Run:
//r0: 原来的16bit内容
    lrw     r2, RunInstRAM
    sth     r0, (r2)

    lrw     r15, bkpt_stack_top

    cmphsi  r0, 0xc000      //判断是否为32为指令
    incf    r2, r2, 2       //不是32位指令，r2+=2
    bf      ll16b           //不是32位指令，跳到后面

    //是32位指令
    ldw     r1, (r15, 196)  //epc
    ldh     r1, (r1, 2)     //加载Flash中的后半字
    sth     r1, (r2, 2)     //保存后半字到RAM程序
ll16b:
    addi    r2, 2           //r2+=2
    mtcr    r2, epc
    stw     r2, (r15, 196)  //写入新epc

    ldm     r0-r13, (r15)   //暂时不恢复r14和r15
    addi    r15, 64
    ldm     r16-r31, (r15)
    addi    r15, 64
    fldms   vr0-vr15, (r15)
    addi    r15, 64

    ldw     r14, (r15)
    mtcr    r14, epsr

    subi    r15,  192
    ldw     r14, (r15, 56)  //恢复r14
    ldw     r15, (r15, 60)  //恢复r15
    rte
    .size   Breakpoint_Handler, . - Breakpoint_Handler
