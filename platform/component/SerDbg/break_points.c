/**********************************************************************
 * Breakpoints operators
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
#include "break_points.h"
#include "reg_ram.h"
#include "wm_hal.h"

extern SDB_RegSave serdbg_regsave;
extern void Error_Handler(void);

int bkpt_last = -1;
BreakPoint bkpts[SERDBG_MAX_BKPT];


uint16_t write_0x0000(uint16_t *p)
{
    uint32_t offaddr = (unsigned)p & (INSIDE_FLS_BASE_ADDR - 1);
    uint32_t buff_addr = RSA_BASE;

    FLASH->CMD_INFO = 0x6;
    FLASH->CMD_START = FLASH_CMD_START_CMD;
    M32(buff_addr) = 0x00000000;

    FLASH->CMD_INFO = 0x80009002 | ((2 - 1) << 16);
    FLASH->ADDR = (offaddr & 0x1FFFFFF);
    FLASH->CMD_START = FLASH_CMD_START_CMD;


    FLASH->CMD_INFO = 0x4;
    FLASH->CMD_START = FLASH_CMD_START_CMD;
    *(uint32_t *)(0xE000F004) = (((uint32_t)p)&0xfffffff0) | (1<<1); //无效缓存行
    return *p;
}

int find_FreeBKPT()
{
    int i;
    for(i=bkpt_last+1;i<SERDBG_MAX_BKPT;i++){
    if(bkpts[i].mode == BKPT_None){
            return i;
        }
    }
    for(i=0;i<=bkpt_last;i++){
        if(bkpts[i].mode == BKPT_None){
            return i;
        }
    }
    return -1;
}

int New_BreakPoint(void *p, BKPT_Mode mode)
{
    if((uint32_t)p & 0x01){
        return -SDB_INVAILD_PTR;
    }
    int index = find_FreeBKPT();
    if(index < 0){
        return -SDB_BKPT_FULL;
    }
    bkpts[index].p = p;
    bkpts[index].old = *(uint16_t*)p;
    bkpts[index].mode = mode;
    bkpt_last = index;
    write_0x0000((uint16_t*)p);
    return index;
}

SerDbg_Stat Erase_BreakPoint(int index)
{
    if(bkpts[index].mode == BKPT_None){
        return SDB_INVAILD_BKPT;
    }
    uint16_t *p = bkpts[index].p;
    if(HAL_FLASH_Write(p, (uint8_t*)&bkpts[index].old, 2) != HAL_OK){
        return SDB_FLASH_OP_ERR;
    }
    *(uint32_t *)(0xE000F004) = (((uint32_t)p)&0xfffffff0) | (1<<1); //无效缓存行
    bkpts[index].mode = BKPT_None;
    return SDB_OK;
}

int find_bkpt_num(void *p)
{
    for(int i=0;i<SERDBG_MAX_BKPT;i++){
        if(bkpts[i].p == p){
            return i;
        }
    }
    return -1;
}

uint16_t Breakpoint_Handler_C(SDB_RegSave* regs)
{
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    int index = find_bkpt_num(regs->epc);
    if(index < 0){
        Error_Handler();
        return 0x0000U;
    }
    BKPT_Mode mode = bkpts[index].mode;
    if(mode == BKPT_BinCmd){
        printf("\n+SDB:B%d\n", index);
        serdbg_parser_cmd();
    }
    if(mode >= BKPT_StrNum){
        printf("BKPT:%d\n", index);
    }
    if(mode >= BKPT_StrRegBase){
        for(int i=0;i<15;i++){
            if(i<10)
                printf(" ");
            printf("r%d:%08x, ", i, regs->rx[i]);
            if((i%4) == 3){
                printf("\n");
            }
        }
        printf("r15:%08x\n", regs->rx[15]);
    }
    if(mode >= BKPT_StrRegAll){
        for(int i=16;i<31;i++){
            if(i<10)
                printf(" ");
            printf("r%d:%08x, ", i, regs->rx[i]);
            if((i%4) == 3){
                printf("\n");
            }
        }
        printf("r31:%08x\n", regs->rx[15]);
        for(int i=0;i<15;i++){
            if(i<10)
                printf(" ");
            printf("vr%d:%08x, ", i, regs->vrx[i]);
            if((i%4) == 3){
                printf("\n");
            }
        }
        printf("vr15:%08x\n", regs->vrx[15]);
    }
    if(mode >= BKPT_StrRegBase){
        printf("epsr: %08x\n", regs->epsr);
        printf("epc : %08x\n", regs->epc);
    }
    return bkpts[index].old;
}

void SDB_New_BKPT_op()
{
    void *p;
    uint8_t tmp;
    SERDBG_RECV(&p, sizeof(p));
    SERDBG_RECV(&tmp, 1);
    tmp = New_BreakPoint(p, tmp);
    SERDBG_SYNC();
    SERDBG_SEND(&tmp, 1);
    SERDBG_SEND(&bkpts[tmp].old, 2);
    SERDBG_SEND(p, 2);
}

void SetMode_BreakPoints_All(BKPT_Mode mode)
{
    for(int i=0;i<SERDBG_MAX_BKPT;i++){
        if(bkpts[i].mode != BKPT_None){
            if(mode == BKPT_None){
                Erase_BreakPoint(i);
            }else{
                bkpts[i].mode = mode;
            }
        }
    }
}

void SDB_Mode_BKPT_op()
{
    uint8_t i;
    uint8_t tmp;
    SERDBG_RECV(&i, sizeof(i));
    SERDBG_RECV(&tmp, sizeof(tmp));
    if(i == 0xff){
        SetMode_BreakPoints_All(tmp);
        tmp = 0x04;  //设置所有使用中的断点
    }else if(i >= SERDBG_MAX_BKPT){
        tmp = 0x03;  //无效编号：超过最大断点数
    }else if(bkpts[i].mode == BKPT_None){
        tmp = 0x02;  //无效编号：该断点未使用
    }else if(tmp == BKPT_None){
        Erase_BreakPoint(i);
        tmp = 0x01;  //清除该断点
    }else{
        bkpts[i].mode = tmp;
        tmp = 0x00;  //设置模式
    }
    SERDBG_SYNC();
    SERDBG_SEND(&tmp, sizeof(tmp));
}
