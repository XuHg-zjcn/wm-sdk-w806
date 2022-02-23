/**********************************************************************
 * @file serdbg.c
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 **********************************************************************
 * Serial debug
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
#include "serdbg.h"
#include "break_points.h"


void serdbg_parser_cmd()
{
    SerDbg_Cmd cmd = 0;
    SERDBG_RECV(&cmd, 1);
    switch(cmd){
        case SDB_Read_Reg:
            SDB_Read_Reg_op();
            break;
        case SDB_Write_Reg:
            SDB_Write_Reg_op();
            break;
        case SDB_Read_Mem:
            SDB_Read_Mem_op();
            break;
        case SDB_Write_Mem:
            SDB_Write_Mem_op();
            break;
        case SDB_Set_BKPT:
            SDB_Set_BKPT_op();
            break;
        case SDB_Unset_BKPT:
            SDB_Unset_BKPT_op();
            break;
        default:
            break;
    }
}