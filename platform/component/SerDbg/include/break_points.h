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