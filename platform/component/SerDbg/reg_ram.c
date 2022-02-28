/**********************************************************************
 * read/write reg and ram
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
#include "reg_ram.h"
#include "serdbg.h"

SDB_RegSave serdbg_regsave;

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
	SERDBG_SYNC();
    if((op.rx < op.ry) && (op.ry <= 50)){
        SERDBG_SEND((uint32_t *)&serdbg_regsave+op.rx, (op.ry-op.rx)*4);
    }
}

// write r14(sp) is invaild
SerDbg_Stat SDB_Write_Reg_op()
{
    RegOp op;
    SERDBG_RECV(&op, sizeof(op));
    if((op.rx < op.ry) && (op.ry <= 50)){
        SERDBG_RECV((uint32_t *)&serdbg_regsave+op.rx, (op.ry-op.rx)*4);
    }
}

SerDbg_Stat SDB_Read_Mem_op()
{
    MemFlashOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_SYNC();
    SERDBG_SEND(op.addr, op.size);
}

SerDbg_Stat SDB_Write_Mem_op()
{
    MemFlashOp op;
    SERDBG_RECV(&op, sizeof(op));
    SERDBG_RECV(op.addr, op.size);
}