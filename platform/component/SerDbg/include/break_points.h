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

typedef struct{
    void *p;        //指针
    uint16_t old;   //原有代码字节
    uint8_t isSTOP; //是否停止
}BreakPoint;

SerDbg_Stat debug_on();
int set_BreakPoint(void *p);
SerDbg_Stat unset_BreakPoint(int x);
SerDbg_Stat clear_All_BreakPoints();
SerDbg_Stat debug_off();

void SDB_Set_BKPT_op();
void SDB_Unset_BKPT_op();

#endif