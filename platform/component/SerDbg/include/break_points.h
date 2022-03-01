/**********************************************************************
 * Breakpoints header file
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

#ifndef BREAK_POINTS_H
#define BREAK_POINTS_H

#include <stdint.h>
#include "serdbg.h"

#define SERDBG_MAX_BKPT 8

typedef enum{
    BKPT_None = 0,       //没有设置过断点或已恢复原样
    BKPT_Ignore = 1,     //忽略断点，但不擦除
    BKPT_BinCmd = 2,     //进入二进制交互模式，需要配合上位机程序使用
    BKPT_StrNum = 3,     //字符串输出断点编号
    BKPT_StrRegBase = 4, //字符串输出基本寄存器(r0-r15, epsr, epc)
    BKPT_StrRegAll = 5,  //字符串输出全部寄存器(r0-r31, vr0-vr15, epsr, epc)
}BKPT_Mode;

typedef struct{
    void *p;        //指针
    uint16_t old;   //原有代码字节
    BKPT_Mode mode; //断点的模式
}BreakPoint;

int New_BreakPoint(void *p, BKPT_Mode mode);
SerDbg_Stat Erase_BreakPoint(int index);
int find_bkpt_num(void *p);

void SetMode_BreakPoints_All(BKPT_Mode mode);
void SDB_New_BKPT_op();
void SDB_Mode_BKPT_op();

#endif
