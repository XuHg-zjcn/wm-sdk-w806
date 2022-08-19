/**************************************************************************
 * Copyright 2021-2022 Xu Ruijun
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************/
#include "atcmd_proc.h"
#include "wm_hal.h"
#include "reg_ram.h"


extern SDB_RegSave serdbg_regsave;


static int atcmd_parser_sdb_proc(int argc, char *argv[], char op)
{
    //TODO: 修改AT指令解析器，不要读取剩余缓冲区，退出中断。
    //这一才能保证下一个字符一定能被读取
    serdbg_regsave.stat = SDB_UARTCap;
    //下一次串口中断时保存寄存器，并进入serdbg_paser_cmd()函数
    //详细请见"uart0_irqhandler.S"
}

static int atcmd_parser_reset_proc(int argc, char *argv[], char op)
{
    CLEAR_REG(RCC->RST);                     // reset all peripherals
    uint32_t rv = *(uint32_t*)(0x00000000U); // get reset vector
    ((void (*)())(rv))();                    // go to ROM
	return 0;
}

atcmd_proc_t atcmd_proc_table[] =
{
    { "SDB",     atcmd_parser_sdb_proc},
    { "Z",       atcmd_parser_reset_proc},
    { NULL,      NULL },
};
