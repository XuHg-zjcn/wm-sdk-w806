/**
 * @file serdbg.c
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "serdbg.h"



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
        default:
            break;
    }
}