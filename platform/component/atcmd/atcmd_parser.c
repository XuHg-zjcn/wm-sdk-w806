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
#include "atcmd_parser.h"
#include "atcmd_proc.h"
#include <string.h>
#include "wm_hal.h"

/*
 * AT+XXX        no-param
 * AT+XXX=a,b,c  setting
 * AT+XXX=?      get-format
 * AT+XXX?       get-stat
 * AT+XXX!
 */

#define ATCMD_PARSER_TIMEOUT       5


extern UART_HandleTypeDef     huart0;

#define __ATCMD_PARSER_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->INTS |= __FLAG__)
#define ATCMD_PARSER_RECV(p, size)  HAL_UART_Receive(&huart0, p, size, ATCMD_PARSER_TIMEOUT)
#define ATCMD_PARSER_SEND(p, size)  HAL_UART_Transmit(&huart0, p, size, ATCMD_PARSER_TIMEOUT)

/* error code */



extern atcmd_proc_t  atcmd_proc_table[];

uint32_t auto_dl_act_ts = 0;


/*ATCMD resource*/
#define ATCMD_PARSER_BUF_SIZE      (256)
uint8_t atcmd_parser_buf[ATCMD_PARSER_BUF_SIZE];
uint32_t atcmd_parser_buf_pt;

atcmd_stat atcmd_parser_exec(char* buf, uint32_t lname, uint32_t len, uint32_t proc_i)
{
    int argc = 1;
    char *argv[ATCMD_PROC_MAX_ARGC];
    char opc = buf[lname];
    argv[0] = &buf[lname+1];
    //split params
    for(int i=lname;i<len;i++){
        if(buf[i] == ','){
            buf[i] = '\0';
            argc++;
            if(argc >= ATCMD_PROC_MAX_ARGC){
                return atcmd_err_maxargs;
            }
            argv[argc] = &buf[i+1];  //下一个参数的启始地址
        }
    }
    atcmd_proc_table[proc_i].proc_func(argc, argv, opc);
    return atcmd_noerr;
}

atcmd_stat atcmd_parser_enter(char* buf, uint32_t len)
{
    char *p;
    uint32_t lname = 0;
    uint32_t proc_i = 0;
    //remove '\r' '\n' tail
    p = buf + len - 1;
    while(1){
        if(*p == '\r' || *p == '\n'){
            *p = '\0';
        }else{
            break;
        }
        p--;
    }
    //find first '=', '?' or '!'
    p = buf;
    while(1){
        if(*p == '=' || *p == '?' || *p == '!'){
            break;
        }else if(lname >= ATCMD_PROC_NAME_MAX_LEN){
            return atcmd_err_maxnamelen;
        }else if(lname >= len){
            break;
        }else{
            p++;
            lname++;
        }
    }
    //find proc
    while(1){
        if(!atcmd_proc_table[proc_i].name){
            break;
        }else if(strcmp(atcmd_proc_table[proc_i].name, buf) == 0){
            atcmd_parser_exec(buf, lname, len, proc_i);
            break;
        }else if(atcmd_proc_table[proc_i].name != NULL){
            proc_i++;
        }else{
            return atcmd_err_notfound;
        }
    }
}


//复制于IOsetting/wm-sdk-w806/platform/component/auto_dl/auto_dl.c
__attribute__((weak)) void USER_UART0_RX(uint8_t ch)
{
    UNUSED(ch);
}

void atcmd_parser_IRQHandler(UART_HandleTypeDef* huart)
{
    uint8_t ch, count;
    uint32_t ts, isrflags = READ_REG(huart->Instance->INTS), isrmasks = READ_REG(huart->Instance->INTM);
    // Clear interrupts
    __ATCMD_PARSER_CLEAR_FLAG(huart, isrflags);

    if (((isrflags & UART_RX_INT_FLAG) != RESET) && ((isrmasks & UART_RX_INT_FLAG) == RESET))
    {
        /**
         *   1) Data always comes in as single bytes, so the count is always 1(or 0);
         *   2) Each byte will comes in twice, the second time with count=0 will be ignored;
         */
        count = ((READ_REG(huart->Instance->FIFOS) & UART_FIFOS_RFC) >> UART_FIFOS_RFC_Pos);
        while (count-- > 0)
        {
            // Write ch to ring buffer
            ch = (uint8_t)(huart->Instance->RDW);
            atcmd_parser_buf[atcmd_parser_buf_pt++] = ch;
            if (atcmd_parser_buf_pt == ATCMD_PARSER_BUF_SIZE) atcmd_parser_buf_pt = 0;

            // Command detection
            ts = HAL_GetTick();
            if ((ts - auto_dl_act_ts) > ATCMD_PARSER_TIMEOUT)
            {
                // Restart the comparison if timeout
                atcmd_parser_buf_pt = 0;
                atcmd_parser_buf[atcmd_parser_buf_pt++] = ch;
            }
            else if (atcmd_parser_buf_pt > 3 && \
                   atcmd_parser_buf[0] == 'A' && \
                   atcmd_parser_buf[1] == 'T' && \
                   atcmd_parser_buf[2] == '+' && \
                   atcmd_parser_buf[atcmd_parser_buf_pt - 1 ] == '\n')
            {
                //TODO: proc in FreeRTOS with macro switch
                atcmd_parser_enter(atcmd_parser_buf+3, atcmd_parser_buf_pt-3);
            }
            // Record last active timestamp
            auto_dl_act_ts = ts;
            USER_UART0_RX(ch);
        }
    }
}
