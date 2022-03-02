/**********************************************************************
 * read/write reg and ram header file
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
#ifndef RW_RAM_H
#define RW_RAM_H

#include "serdbg.h"

typedef enum{
    Norm = 0,
    ITrack = 1,
    Undef = 2,
    JTrack = 3,
}PSR_TM;

#pragma pack(1)
typedef struct{
    _Bool C:1;
    int resv1:5;
    _Bool IE:1;
    _Bool IC:1;
    _Bool EE:1;
    _Bool MM:1;
    int resv2:4;
    PSR_TM TM:2;
    uint8_t VEC:8;
    int resv3:7;
    _Bool S:1;
}EPSR_t;
#pragma pack()

typedef enum{
    SDB_NoWait = 0,
    SDB_UARTCap = 1,
    SDB_StepStop = 2
}SDB_RegSaveStat;

#pragma pack(4)
typedef struct{
    uint32_t rx[32];
    uint32_t vrx[16];
    EPSR_t epsr;
    void *epc;
    SDB_RegSaveStat stat;
}SDB_RegSave;
#pragma pack()

SerDbg_Stat SDB_Read_Reg_op();
SerDbg_Stat SDB_Write_Reg_op();
SerDbg_Stat SDB_Read_Mem_op();
SerDbg_Stat SDB_Write_Mem_op();

//待实现
SerDbg_Stat SDB_Read_Flash_op();
SerDbg_Stat SDB_Write_Flash_op();

#endif
