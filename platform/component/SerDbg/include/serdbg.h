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

typedef enum{
    SDB_OK = 0,
    SDB_INVAILD_PTR,
    SDB_INVAILD_BKPT,
    SDB_BKPT_FULL,
    SDB_FLASH_OP_ERR,
}SerDbg_Stat;

#endif