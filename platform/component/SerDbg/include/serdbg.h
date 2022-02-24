/*****************************************************************************
 * @file serdbg.h
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief 
 * @version 0.1
 * @date 2021-11-27
 * @note 请勿把断点设置在本程序内，可能发生错误
 *       如果不把所有断点清除，复位后会CRC校验错误而无法启动，需要重新下载程序
 *       请把flash擦写函数以及所依赖的函数放在单独一个块内，防止擦除自身
 *****************************************************************************
 *  serial debug
 *  Copyright (C) 2022  Xu Ruijun
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *****************************************************************************/
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

typedef enum{           // Param,                       | ret
    SDB_Read_Reg   = 0, // rx-ry(2B)                    | data(4nB)
    SDB_Write_Reg  = 1, // rx-ry(2B), data(4nB)         | stat(1B)
    SDB_Read_Mem   = 2, // addr(4B), len(2B)            | data(len), stat(1B)
    SDB_Write_Mem  = 3, // addr(4B), len(2B), data(lenB)| stat(1B)
    SDB_Read_Flash = 4, //
    SDB_Write_Flash= 5, //
    SDB_Set_BKPT   = 6, // addr(4B)                     | index(1B)
    SDB_Upd_BKPT   = 7, // index(1B), set(1B)           | stat(1B)
    SDB_Get_BKPTS  = 8, //                              | n(1B), set(nB)
    SDB_Unset_BKPT = 9, // index(1B)                    | stat(1B)
    SDB_Clear_BKPT = 10,//                              | stat(1B)
    SDB_Contin_BKPT= 11,//                              |
    SDB_Pause      = 12,//                              |
    SDB_Resume     = 13,//                              |
    SDB_About      = 14,//                              | str
    SDB_EXIT       = 15,//                              | stat(1B)
}SerDbg_Cmd;

void serdbg_parser_cmd();

#endif