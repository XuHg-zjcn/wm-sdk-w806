#include "reg_ram.h"
#include "serdbg.h"

extern uint32_t* uart0_irq_sp;

#pragma pack(1)
typedef struct{
    uint8_t rx;
    uint8_t ry;
}RegOp;

typedef struct
{
    uint8_t *addr;
    uint16_t size;
}MemFlashOp;
#pragma pack()

// only support uart inttrupt
// TODO: add read register in breakpoint
SerDbg_Stat SDB_Read_Reg_op()
{
    RegOp op;
    SERDBG_RECV(&op, sizeof(op));
    if(uart0_irq_sp && (op.rx < op.ry) && (op.ry <= 50)){
        SERDBG_SEND(uart0_irq_sp+op.rx, (op.ry-op.rx)*4);
    }
}

// write r14(sp) is invaild
SerDbg_Stat SDB_Write_Reg_op()
{
    RegOp op;
    SERDBG_RECV(&op, sizeof(op));
    if(uart0_irq_sp && (op.rx < op.ry) && (op.ry <= 50)){
        SERDBG_RECV(uart0_irq_sp+op.rx, (op.ry-op.rx)*4);
    }
}

SerDbg_Stat SDB_Read_Mem_op()
{
    MemFlashOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_SEND(op.addr, op.size);
}

SerDbg_Stat SDB_Write_Mem_op()
{
    MemFlashOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_RECV(op.addr, op.size);
}