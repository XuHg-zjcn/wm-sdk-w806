#include "reg_ram.h"
#include "serdbg.h"

typedef struct
{
    uint8_t *addr;
    uint16_t size;
}MemOp;

void SDB_Read_aReg_proc()
{
    uint8_t rx;
    uint32_t data;
    SERDBG_RECV(&rx, 1);
    switch (rx){
        case 49:
            __asm__("mfcr %0, epsr" :"=r"(data));
            break;
        case 50:
            __asm__("mfcr %0, epc" :"=r"(data));
            break;
        default:
            return -1;
    }
    SERDBG_SEND(&data, 4);
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