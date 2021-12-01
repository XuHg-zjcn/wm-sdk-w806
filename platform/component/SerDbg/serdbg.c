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
        case SDB_Read_aReg:
            SDB_Read_aReg_proc();
            break;
        case SDB_Read_Mem:
            SDB_Read_Mem_proc();
            break;
    }
}