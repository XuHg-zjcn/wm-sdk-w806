/**
 * @file serdbg.S
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

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
/**
 * load regs and run
 */
    .align  2
    .global Load_Regs_Run
    .type   Load_Regs_Run, %function
Load_Regs_Run:
//r0: 原来的16bit内容
    lrw     r2, RunInstRAM
    sth     r0, (r2)
    mtcr    r2, epc

    lrw     r15, bkpt_stack_top
    ldw     r3,  (r15, 196)
    addi    r3,  2

    cmphsi  r0, 0x8000
    movi    r1, 0x4000      //恢复空指令
    bf      ll16b           //不是32位指令，跳过步骤

    //是32位指令
    ldh     r1, (r3)        //加载Flash中的后半字
    addi    r3,  2          //epc应该再+2

ll16b:
    sth     r1, (r2, 2)     //把后半字写入RAM的程序中
    stw     r3, (r15, 196)  //写入新epc

    ldm     r0-r13, (r15)
    addi    r15, 64
    ldm     r16-r31, (r15)
    addi    r15, 64
    fldms   vr0-vr15, (r15)
    addi    r15, 64

    ldw     r14, (r15)
    mtcr    r14, epsr

    subi    r15,  192
    ldw     r14, (r15, 56)
    ldw     r15, (r15, 60)
    rte
    .size   Load_Regs_Run, . - Load_Regs_Run


    .align  2
    .global Breakpoint_Handler
    .type   Breakpoint_Handler, %function
Breakpoint_Handler:
    //把r15 PUSH到栈
    subi    sp, 4
    stw     r15, (sp)
    addi    sp, 4      //恢复原来栈指针，供下面保存

    //保存r0-r14(sp)
    lrw     r15, bkpt_stack_top
    stm     r0-r14, (r15)

    //保存刚才栈里的r15
    subi   sp, 4
    ldw    r13, (sp)
    stw    r13, (sp, 60)

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
    mov    r0,  r15    //r0参数

    lrw    r15, Load_Regs_Run   //C函数结束后返回
    lrw    r4,  Breakpoint_Handler_C
    jmp    r4   //跳转到C函数
    .size   Breakpoint_Handler, . - Breakpoint_Handler
