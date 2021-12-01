/**
 * @file atcmd_parser.c
 * @author Xu Ruijun | 1687701765@qq.com
 * @brief AT commands parser
 * @version 0.1
 * @date 2021-11-30
 * @note ref by W800 SDK "factory_atcmd.c"
 * @note format "AT+{CommdName} [Para1] [Para2]\r\n",
 *       proc will call after "\r\n" recvied,
 *       proc will in interrupt, shoun't much time.
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "atcmd_parser.h"
#include "atcmd_proc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "wm_hal.h"


#define ATCMD_PARSER_TIMEOUT       5


#define ATCMD_PARSER_OP_NULL       0
#define ATCMD_PARSER_OP_EQ         1    /* = */
#define ATCMD_PARSER_OP_EP         2    /* =! , update flash*/
#define ATCMD_PARSER_OP_QU         3    /* =? */

extern UART_HandleTypeDef     huart0;

#define __ATCMD_PARSER_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->INTS |= __FLAG__)
#define ATCMD_PARSER_RECV(p, size)  HAL_UART_Receive(&huart0, p, size, ATCMD_PARSER_TIMEOUT)
#define ATCMD_PARSER_SEND(p, size)  HAL_UART_Transmit(&huart0, p, size, ATCMD_PARSER_TIMEOUT)

/* error code */



extern struct atcmd_parser_t  atcmd_parser_tbl[];

uint32_t auto_dl_act_ts = 0;


/*ATCMD resource*/
#define ATCMD_PARSER_BUF_SIZE     (256)
#define ATCMD_PARSER_RESPONSE_SIZE (512)
static char *factory_cmdrsp_buf = NULL;
uint8_t atcmd_parser_buf[ATCMD_PARSER_BUF_SIZE];
uint32_t atcmd_parser_buf_pt;



static int atcmd_parser_nop_proc(struct atcmd_parser_token_t *tok,
                          char *res_resp, u32 *res_len)
{
    if (!tok->arg_found && (tok->op == ATCMD_PARSER_OP_NULL))
    {
        *res_len = atcmd_parser_ok_resp(res_resp);
    }
    else
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_OPS);
    }

    return 0;
}

static int atcmd_parser_parse(struct atcmd_parser_token_t *tok, char *buf, u32 len)
{
    char *c, *end_line, *comma;
    int remain_len;
    char *buf_start = buf;

    /* at command "AT+", NULL OP */
    if (len == 0)
    {
        *tok->name = '\0';
        tok->arg_found = 0;
        return -1;
    }

    /* parse command name */
    c = strchr(buf, '=');
    if (!c)
    {
        /* format :  at+wprt */
        c = strchr(buf, '\n');
        if (!c)
            return -ATCMD_PARSER_ERR_INV_FMT;
        if ((c - buf) > (ATCMD_PARSER_NAME_MAX_LEN - 1))
            return -ATCMD_PARSER_ERR_UNSUPP;
        memcpy(tok->name, buf, c - buf);
        *(tok->name + (c - buf)) = '\0';
        tok->op = ATCMD_PARSER_OP_NULL;
        tok->arg_found = 0;
        return 0;
    }
    else
    {
        /* format : at+wprt=0
         *          at+skct=0,0,192.168.1.4,80 */
        if ((c - buf) > (ATCMD_PARSER_NAME_MAX_LEN - 1))
            return -ATCMD_PARSER_ERR_UNSUPP;
        memcpy(tok->name, buf, c - buf);
        *(tok->name + (c - buf)) = '\0';
        tok->op = ATCMD_PARSER_OP_NULL;
        buf += (c - buf + 1);
        switch(*buf)
        {
        case '!':
            tok->op = ATCMD_PARSER_OP_EP;
            buf++;
            break;
        case '?':
            tok->op = ATCMD_PARSER_OP_QU;
            buf++;
            break;
        default:
            tok->op = ATCMD_PARSER_OP_EQ;
            break;
        }
        tok->arg[0] = buf;
        tok->arg_found = 0;
        remain_len = len - (buf - buf_start);
        end_line = strchr(buf, '\n');
        if (!end_line)
            return -ATCMD_PARSER_ERR_INV_FMT;
        while (remain_len > 0)
        {
            comma = strchr(buf, ',');
            if (end_line && !comma)
            {
                if (tok->arg_found >= (ATCMD_PARSER_MAX_ARG - 1))
                    return -ATCMD_PARSER_ERR_INV_PARAMS;
                if (end_line != buf)
                    tok->arg_found++;
                /* last parameter */
                *(u8 *)end_line = '\0';
                tok->arg[tok->arg_found] = end_line + 1;
                remain_len -= (end_line - buf);
                if (remain_len > 1)
                    return -ATCMD_PARSER_ERR_NOT_ALLOW;
                else
                    return 0;
            }
            else
            {
                if (tok->arg_found >= (ATCMD_PARSER_MAX_ARG - 1))
                    return -ATCMD_PARSER_ERR_INV_PARAMS;
                tok->arg_found++;
                *(u8 *)comma = '\0';
                tok->arg[tok->arg_found] = comma + 1;
                remain_len -= (comma - buf + 1);
                buf = comma + 1;
            }
        }
        return 0;
    }
}

static int atcmd_parser_exec(struct atcmd_parser_token_t *tok, char *res_rsp, u32 *res_len)
{
    int err;
    struct atcmd_parser_t *atcmd, *match = NULL;
    int i = 0;
    int name_len = strlen(tok->name);

    for (i = 0; i < name_len; i++)
        tok->name[i] = toupper(tok->name[i]);

    if (strlen(tok->name) == 0)
    {
        err = atcmd_parser_nop_proc(tok, res_rsp, res_len);
        return err;
    }
    /* look for AT CMD handle table */
    atcmd = atcmd_parser_tbl;
    while (atcmd->name)
    {
        if (strcmp(atcmd->name, tok->name) == 0)
        {
            match = atcmd;
            break;
        }
        atcmd++;
    }

    /* at command handle */
    if (match)
    {
        //TLS_DBGPRT_INFO("command find: %s\n", atcmd->name);
        err = match->proc_func(tok, res_rsp, res_len);
    }
    else
    {
        /* at command not found */
        *res_len = sprintf(res_rsp, "+ERR=%d", -ATCMD_PARSER_ERR_UNSUPP);
        err = -ATCMD_PARSER_ERR_UNSUPP;
    }

    return err;
}


int atcmd_parser_enter(char *charbuf, unsigned char charlen)
{
    struct atcmd_parser_token_t atcmd_tok;
    int err;
    u32 cmdrsp_size;

    if ((charlen >= 2) && (charbuf[charlen - 2] == '\r' || charbuf[charlen - 2] == '\n'))
    {
        charbuf[charlen - 2] = '\n';
        charbuf[charlen - 1] = '\0';
        charlen = charlen - 1;
    }
    else if ((charlen >= 1) && (charbuf[charlen - 1] == '\r' || charbuf[charlen - 1] == '\n'))
    {
        charbuf[charlen - 1] = '\n';
        charbuf[charlen] = '\0';
        charlen = charlen;
    }
    else
    {
        charbuf[charlen] = '\n';
        charbuf[charlen + 1] = '\0';
        charlen = charlen + 1;
    }

    memset(&atcmd_tok, 0, sizeof(struct atcmd_parser_token_t));

    err = atcmd_parser_parse(&atcmd_tok, (char *)charbuf, charlen);
    if (err)
        return -1;

    err = atcmd_parser_exec(&atcmd_tok, factory_cmdrsp_buf, &cmdrsp_size);
    if (err && (err != -ATCMD_PARSER_ERR_UNSUPP))
    {
        return -1;
    }

	if (cmdrsp_size < (ATCMD_PARSER_RESPONSE_SIZE - 5))
	{
	    factory_cmdrsp_buf[cmdrsp_size] = '\r';
	    factory_cmdrsp_buf[cmdrsp_size + 1] = '\n';
	    factory_cmdrsp_buf[cmdrsp_size + 2] = '\r';
	    factory_cmdrsp_buf[cmdrsp_size + 3] = '\n';	
	    factory_cmdrsp_buf[cmdrsp_size + 4] = '\0';
		cmdrsp_size += 4;

		ATCMD_PARSER_SEND(factory_cmdrsp_buf, cmdrsp_size);
	}
	else
	{
		
	}

    return 0;
}

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

__attribute__((isr)) void UART0_IRQHandler(void)
{
    atcmd_parser_IRQHandler(&huart0);
}