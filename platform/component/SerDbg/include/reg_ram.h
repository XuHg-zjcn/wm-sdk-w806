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

typedef struct{
    uint32_t rx[32];
    uint32_t vrx[16];
    uint32_t epsr;
    void *epc;
}SDB_RegSave;

SerDbg_Stat SDB_Read_Reg_op();
SerDbg_Stat SDB_Write_Reg_op();
SerDbg_Stat SDB_Read_Mem_op();
SerDbg_Stat SDB_Write_Mem_op();

//待实现
SerDbg_Stat SDB_Read_Flash_op();
SerDbg_Stat SDB_Write_Flash_op();

#endif