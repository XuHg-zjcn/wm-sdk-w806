/**
 * @file serdbg.h
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 * @note 请勿把断点设置在本程序内，可能发生错误
 *       如果不把所有断点清除，复位后会CRC校验错误而无法启动，需要重新下载程序
 *       请把flash擦写函数以及所依赖的函数放在单独一个块内，防止擦除自身
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef SERDBG_H
#define SERDBG_H

#include <stdint.h>
#include "wm_hal.h"

extern UART_HandleTypeDef huart0;
#define SERDBG_TIMEOUT  5
#define SERDBG_SEND(p, size)  HAL_UART_Transmit(&huart0, (uint8_t*)(p), (size), SERDBG_TIMEOUT)
#define SERDBG_RECV(p, size)  HAL_UART_Receive(&huart0, (uint8_t*)(p), (size), SERDBG_TIMEOUT)

typedef enum{
    SDB_OK = 0,
    SDB_INVAILD_PTR,
    SDB_INVAILD_BKPT,
    SDB_BKPT_FULL,
    SDB_FLASH_OP_ERR,
    SDB_CRC_ERR
}SerDbg_Stat;

typedef enum{           // Param,                         ret
    SDB_Read_Regs = 0,  //                              | data(4*50B)
    SDB_Write_Regs,     // data(4*50B)                  | stat(1B)
    SDB_Read_aReg,      // rx(1B)                       | data(4B)
    SDB_Write_aReg,     // rx(1B), data(4B)             | stat(1B)
    SDB_Read_Mem,       // addr(4B), len(2B)            | data(len), stat(1B)
    SDB_Write_Mem,      // addr(4B), len(2B), data(lenB)| stat(1B)
    SDB_Set_BKPT,       // addr(4B)                     | index(1B)
    SDB_Upd_BKPT,       // index(1B), set(1B)           | stat(1B)
    SDB_Get_BKPTS,      //                              | n(1B), set(nB)
    SDB_Unset_BKPT,     // index(1B)                    | stat(1B)
    SDB_Clear_BKPT,     //                              | stat(1B)
    SDB_Contin_BKPT,    //                              |
    SDB_About,          //                              | str
    SDB_EXIT            //                              | stat(1B)
}SerDbg_Cmd;

void serdbg_parser_cmd();

#endif