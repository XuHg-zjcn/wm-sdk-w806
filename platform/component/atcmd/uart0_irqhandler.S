.section .bss
    .global uart0_irq_sp
uart0_irq_sp:
    .long 0

.text
    .align  2
    .global UART0_IRQHandler
    .type   UART0_IRQHandler, %function
UART0_IRQHandler:
    nie
    subi      sp,  64
    fstms     fr0-fr15, (sp)
    subi      sp,  64
    stm       r16-r31, (sp)
    subi      sp,  64
    stm       r0-r13,  (sp)
    stw       r15,  (sp, 60)

   //保存栈指针
    addi      r0, sp, 200
    stw       r0, (sp, 56)
    lrw       r4, uart0_irq_sp
    stw       sp, (r4)

    lrw       r0,  huart0
    bsr       atcmd_parser_IRQHandler

   //清除栈指针
    movi      r0,  0
    stw       r0,  (r4)

    ldm       r0-r13,  (sp)
    ldw       r15,  (sp, 60)
    addi      sp,  64
    ldm       r16-r31,  (sp)
    addi      sp,  64
    fldms     fr0-fr15,  (sp)
    addi      sp,  64
    nir