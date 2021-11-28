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

int set_BreakPoint(void *p)
{
    if((uint32_t)p & 0x01){
        return -SDB_INVAILD_PTR;
    }if(n_bkpt == SERDBG_MAX_BKPT){
        return -SDB_BKPT_FULL;
    }
    const uint16_t zero = 0;
    if(HAL_FLASH_Write((uint32_t)p, (uint8_t*)&zero, 2) != HAL_OK){
        return -SDB_FLASH_OP_ERR;
    }
    bkpts[n_bkpt].p = p;
    bkpts[n_bkpt].old = *(uint16_t*)p;
    bkpts[n_bkpt].isSTOP = 0;
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
    printf("bkpt index=%d\r\n", index);
    printf("clear breakpoints=%d\r\n", clear_All_BreakPoints());
    while(1);
    if(index < 0){
        Error_Handler();
        return 0x0000U;
    }else{
        return bkpts[index].old;
    }
}