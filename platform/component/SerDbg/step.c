/**********************************************************************
 * Track Handler C part
 * Copyright (C) 2022  Xu Ruijun
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
#include "step.h"
#include "reg_ram.h"


#define MSK_HIGH(msk)     (0x80000000>>__builtin_clz(msk))
#define ZERO_EXT(x, msk)  ((x&msk)>>__builtin_ctz(msk))
#define SIGN_EXT(x, msk)  (ZERO_EXT(x, msk)-((MSK_HIGH(msk)&x)?1U<<__builtin_popcount(msk):0))


extern SDB_RegSave serdbg_regsave;
const uint8_t TrackSync[] = {'\n', '+', 'S', 'D', 'B', ':', 'S', '\n'};
uint32_t RunInstRAM = 0;  //不需要运行指令时必须为0，否则会影响正常运行。
uint32_t RunInstRAM_epc = 0;


void RunStep_RAM()
{
    //以下指令对程序指针(PC)进行了读操作，需要用程序模拟
    //BSR, LRW, JSR
    uint8_t h6 = (RunInstRAM&0xffff)>>10;
    if(h6 == 0b111000) {                                   //bsr32
        serdbg_regsave.rx[15] = serdbg_regsave.epc+4;
        serdbg_regsave.epc += SIGN_EXT(RunInstRAM, 0x03ff)<<17;
        serdbg_regsave.epc += (RunInstRAM>>16)<<1;
    }else if((h6&0b111011) == 0b000000) {                  //lrw16
        uint32_t z = ZERO_EXT(RunInstRAM, 0x00e0);
        uint32_t offset = (ZERO_EXT(RunInstRAM, 0x03)<<5) | (ZERO_EXT(RunInstRAM, 0x1f));
        if(!(h6 & 0b000100)) {
            offset |= 0x80;                                //lrw16-1
        }
        serdbg_regsave.rx[z] += *(uint32_t *)((serdbg_regsave.epc+(offset<<2))&0xfffffffc);
        serdbg_regsave.epc += 2;
    }else if(((RunInstRAM&0xffff)>>5) == 0b11101010100) {  //lrw32
        uint32_t z = ZERO_EXT(RunInstRAM, 0x001f);
        uint32_t offset = RunInstRAM>>16;
        serdbg_regsave.rx[z] += *(uint32_t *)((serdbg_regsave.epc+(offset<<2))&0xfffffffc);
        serdbg_regsave.epc += 4;
    }else if((RunInstRAM&0xffc3) == 0x7bc1) {              //jsr16
        uint32_t x = ZERO_EXT(RunInstRAM, 0x3c);
        uint32_t tmp = serdbg_regsave.epc + 4;  //质疑：手册上是+4，需测试
        serdbg_regsave.epc = serdbg_regsave.rx[x] & 0xfffffffe;
        serdbg_regsave.rx[15] = tmp;
    }else{
        serdbg_regsave.epsr.TM = ITrack;
        RunInstRAM_epc = serdbg_regsave.epc;
        serdbg_regsave.epc = (uint32_t)&RunInstRAM;
        return;
    }
    //上面没有返回
    RunInstRAM = 0;
    RunInstRAM_epc = 0;
    Track_Handler_C(&serdbg_regsave, serdbg_regsave.epc);
}

void RunStep()
{
    serdbg_regsave.stat = SDB_StepStop;
    if(RunInstRAM){
        RunStep_RAM();
    }else{
        serdbg_regsave.epsr.TM = ITrack;
    }
}

void RunResume()
{
    serdbg_regsave.stat = SDB_NoWait;
    if(RunInstRAM){
        RunStep_RAM();
    }else{
        serdbg_regsave.epsr.TM = Norm;
    }
}

void Track_Handler_C(SDB_RegSave *regs, uint32_t epc)
{
    if(RunInstRAM != 0){
        //判断所执行的指令是否发生绝对跳转
        if((RunInstRAM&0xffc3) != 0x7800 && //不是jmp16
           (RunInstRAM&0xffe0) != 0x1480) { //不是pop16
            //没有发生绝对跳转，把差值加到旧的epc上
            epc = RunInstRAM_epc + (epc - (uint32_t)&RunInstRAM);
        }
        RunInstRAM = 0;
        RunInstRAM_epc = 0;
    }
    regs->epc = epc;
    if(regs->stat == SDB_StepStop){
        SERDBG_SEND(TrackSync, sizeof(TrackSync));
        serdbg_parser_cmd();
    }else{
        regs->epsr.TM = Norm;
    }
}
