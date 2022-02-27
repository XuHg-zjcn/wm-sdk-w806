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
    _Bool pause = 0;
    SerDbg_Cmd cmd;
    do{
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
            case SDB_New_BKPT:
                SDB_New_BKPT_op();
                break;
            case SDB_Mode_BKPT:
                SDB_Mode_BKPT_op();
                break;
            case SDB_Pause:
                pause = 1;
	        break;
            case SDB_Resume:
	        pause = 0;
	        break;
            default:
	        return;
	}
    }while(pause);
}
