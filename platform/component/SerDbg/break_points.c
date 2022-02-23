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
#include "wm_hal.h"

//uint8_t dbg_on = 0;
uint8_t n_bkpt = 0;
BreakPoint bkpts[SERDBG_MAX_BKPT];

typedef struct{
    uint32_t rx[32];
    uint32_t vrx[16];
    uint32_t epsr;
    void *epc;
}BKPT_Regs;

uint16_t write_0x0000(uint16_t *p)
{
    uint32_t offaddr = (unsigned)p & (INSIDE_FLS_BASE_ADDR - 1);
    uint32_t secpos = offaddr / FLASH_CMD_START_CMD;
    uint32_t pageaddr = secpos * INSIDE_FLS_SECTOR_SIZE;
    uint32_t u16offset = offaddr % INSIDE_FLS_SECTOR_SIZE;
    uint32_t buff_addr = RSA_BASE;
    uint32_t i = 0;

    FLASH->CMD_INFO = 0x6;
	FLASH->CMD_START = FLASH_CMD_START_CMD;

    M32(buff_addr) = 0x00000000;

    FLASH->CMD_INFO = 0x80009002 | ((2 - 1) << 16);
    FLASH->ADDR = (offaddr & 0x1FFFFFF);
    FLASH->CMD_START = FLASH_CMD_START_CMD;


    FLASH->CMD_INFO = 0x4;
	FLASH->CMD_START = FLASH_CMD_START_CMD;
    return *p;
}

int set_BreakPoint(void *p)
{
    if((uint32_t)p & 0x01){
        return -SDB_INVAILD_PTR;
    }if(n_bkpt == SERDBG_MAX_BKPT){
        return -SDB_BKPT_FULL;
    }
    const uint16_t zero = 0;
    printf("%08x\r\n", (uint32_t)p);
    //wm_flash_unlock();
    //wm_flash_lock();
    bkpts[n_bkpt].p = p;
    bkpts[n_bkpt].old = *(uint16_t*)p;
    bkpts[n_bkpt].isSTOP = 0;
    printf("%04x\r\n", write_0x0000((uint16_t*)p));
    printf("%08x\r\n", (uint32_t)p);
    return n_bkpt++;
}

SerDbg_Stat unset_BreakPoint(int x)
{
    if(x >= n_bkpt){
        return SDB_INVAILD_BKPT;
    }
    if(HAL_FLASH_Write((uint32_t)bkpts[x].p, (uint8_t*)&bkpts[x].old, 2) != HAL_OK){
        return SDB_FLASH_OP_ERR;
    }
    n_bkpt--;
    for(;x<n_bkpt;x++){
        bkpts[x] = bkpts[x+1];
    }
    return SDB_OK;
}

SerDbg_Stat clear_All_BreakPoints()
{
    SerDbg_Stat stat;
    while(n_bkpt){
        stat = unset_BreakPoint(n_bkpt-1);
        if(stat != SDB_OK){
            return stat;
        }
    }
    return SDB_OK;
}

int find_bkpt_num(void *p)
{
    int index = 0;
    while(index < n_bkpt){
        if(bkpts[index].p == p){
            return index;
        }
        index++;
    }
    return -1;
}

uint16_t Breakpoint_Handler_C(BKPT_Regs* regs)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    int index = find_bkpt_num(regs->epc);
    for (uint8_t i = 0; i < 32; i++) {
        printf("r%d: %08x\t", i, regs->rx[i]);
        if ((i % 5) == 4) {
            printf("\n");
        }
    }
    printf("\n");
    for (uint8_t i = 0; i < 16; i++) {
        printf("vr%d: %08x\t", i, regs->vrx[i]);
        if ((i % 5) == 4) {
            printf("\n");
        }
    }
    printf("\n");
    printf("epsr: %08x\n", regs->epsr);
    printf("epc : %08x\n", regs->epc);
    printf("bkpt index=%d\r\n", index);
    //printf("clear breakpoints=%d\r\n", clear_All_BreakPoints());
    if(index < 0){
        Error_Handler();
        return 0x0000U;
    }else{
        printf("old:  %04x\n", bkpts[index].old);
        return bkpts[index].old;
    }
}

void SDB_Set_BKPT_op()
{
    void *p;
    SERDBG_RECV(&p, sizeof(p));
    set_BreakPoint(p);
}

void SDB_Unset_BKPT_op()
{
    uint8_t i;
    SERDBG_RECV(&i, sizeof(i));
    if(i < SERDBG_MAX_BKPT){
        unset_BreakPoint(i);
    }
}