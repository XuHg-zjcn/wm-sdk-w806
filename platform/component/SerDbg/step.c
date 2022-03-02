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
#include "break_points.h"
#include "reg_ram.h"

extern SDB_RegSave serdbg_regsave;

const uint8_t TrackSync[] = {'\n', '+', 'S', 'D', 'B', ':', 'S', '\n'};

void Track_Handler_C(SDB_RegSave *regs)
{
    if(regs->stat == SDB_StepStop){
        regs->epsr.TM = ITrack;
        SERDBG_SEND(TrackSync, sizeof(TrackSync));
        serdbg_parser_cmd();
    }else{
        regs->epsr.TM = Norm;
    }
}
