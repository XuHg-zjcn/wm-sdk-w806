#include "reg_ram.h"
#include "serdbg.h"

extern uint32_t* uart0_irq_sp;

#pragma pack(1)
typedef struct
{
    uint8_t *addr;
    uint16_t size;
}MemOp;
#pragma pack()

void SDB_Read_aReg_proc()
{
    uint8_t rx;
    // only support uart inttrupt
    // TODO: add read register in breakpoint
    if(uart0_irq_sp){
        SERDBG_RECV(&rx, 1);
        SERDBG_SEND(uart0_irq_sp+rx, 4);
    }
}

void SDB_Read_Mem_proc()
{
    MemOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_SEND(op.addr, op.size);
}

void SDB_Write_Mem_proc()
{
    MemOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_SEND(op.addr, op.size);
}