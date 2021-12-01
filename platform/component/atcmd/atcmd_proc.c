#include "atcmd_proc.h"
#include <stdio.h>
#include "wm_hal.h"


const char HwVer[] = "H0.312";
const char FirmWareVer[] = "0.01";

int hex_to_digit(int c)
{
        if( '0' <= c && c <= '9' )
                return c - '0';
        if( 'A' <= c && c <= 'F' )
                return c - ('A' - 10);
        if( 'a' <= c && c <= 'f' )
                return c - ('a' - 10);
        return -1;;
}


int hexstr_to_uinit(char *buf, u32 *d)
{
    int i;
    int len = strlen(buf);
    int c;
    *d = 0;

    if (len > 8)
        return -1;
    for (i = 0; i < len; i++)
    {
        c = hex_to_digit(buf[i]);
        if (c < 0)
            return -1;
        *d = (u8)c | (*d << 4);
    }
    return 0;
}

int atcmd_parser_ok_resp(char *buf)
{
    int len;
    len = sprintf(buf, "+OK");
    return len;
}

int atcmd_parser_err_resp(char *buf, int err_code)
{
    int len;
    len = sprintf(buf, "+ERR=%d", -err_code);
    return len;
}

static int atcmd_parser_sdb_proc(struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    serdbg_parser_cmd();
}

static int atcmd_parser_reset_proc(struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    CLEAR_REG(RCC->RST);                     // reset all peripherals
    uint32_t rv = *(uint32_t*)(0x00000000U); // get reset vector
    ((void (*)())(rv))();                    // go to ROM
	return 0;
}


/******************************************************************
* Description:	Read register or memory

* Format:		AT+&REGR=<address>,[num]<CR>
			+OK=<value1>,[value2]...<CR><LF><CR><LF>

* Argument:	address: num:

* Author: 	kevin 2014-03-19
******************************************************************/
static int atcmd_parser_regr_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret;
    u32 Addr, Num, Value;
    u8 buff[16];

    if(2 != tok->arg_found)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[1], &Num);
    if(ret)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }
    Value = *(uint32_t*)Addr;
    *res_len = sprintf(res_resp, "+OK=%08x", Value);
    memset(buff, 0, sizeof(buff));
    while(--Num)
    {
        Addr += 4;
        Value = *(uint32_t*)Addr;
        *res_len += sprintf((char *)buff, ",%08x", Value);
        strcat(res_resp, (char *)buff);
    }
    return 0;
}

/******************************************************************
* Description:	Write register or memory

* Format:		AT+&REGW=<address>,<value1>,[value2]...<CR>
			+OK=<CR><LF><CR><LF>

* Argument:	address: value:

* Author: 	kevin 2014-03-19
******************************************************************/
static int atcmd_parser_regw_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret;
    u32 Addr, Value, i;

    if((tok->arg_found < 2) || (tok->arg_found > 9))
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_OPS);
        return 0;
    }

    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }

    for(i = 0; i < tok->arg_found - 1; i++)
    {
        ret = hexstr_to_uinit(tok->arg[i + 1], &Value);
        if(ret)
        {
            *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
            return 0;
        }
        else
        {
            *(uint32_t*)Addr = Value;
        }
        Addr += 4;
    }
    *res_len = atcmd_parser_ok_resp(res_resp);
    return 0;
}


//flash read
static int atcmd_parser_flsr_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Num;
    u32 Value;

    if(2 != tok->arg_found)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[1], &Num);
    if(ret || (Num < 1) || (Num > 4))
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }

	*res_len = sprintf(res_resp, "+OK=");
    for(i = 0; i < Num - 1; i++)
    {
		HAL_FLASH_Read(Addr, (u8 *)&Value,4);
		*res_len += sprintf(res_resp + *res_len, "%04x,", Value);
        Addr += 4;
    }
	HAL_FLASH_Read(Addr, (u8 *)&Value,4);
	*res_len += sprintf(res_resp + *res_len, "%04x", Value);

    return 0;
}


//flash
static int atcmd_parser_flsw_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Value;

    if((tok->arg_found < 2) || (tok->arg_found > 9))
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
        return 0;
    }

    for(i = 0; i < tok->arg_found - 1; i++)
    {
        ret = hexstr_to_uinit(tok->arg[i + 1], &Value);
        if(ret)
        {
            *res_len = atcmd_parser_err_resp(res_resp, ATCMD_PARSER_ERR_INV_PARAMS);
            return 0;
        }
        else
        {
			HAL_FLASH_Write(Addr, (u8 *)&Value, 4);
	        Addr += 4;
        }
    }

    *res_len = atcmd_parser_ok_resp(res_resp);
    return 0;
}


/*dummy function*/
static int atcmd_parser_wbgr_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
	*res_len = atcmd_parser_ok_resp(res_resp);
	return 0;
}

static int atcmd_parser_qver_proc( struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
	*res_len = sprintf(res_resp, "+OK=%c%x.%02x.%02x.%02x%02x,%c%x.%02x.%02x@ %s %s",
                HwVer[0], HwVer[1], HwVer[2],HwVer[3], HwVer[4], HwVer[5], \
                FirmWareVer[0], FirmWareVer[1], FirmWareVer[2],FirmWareVer[3], \
                __DATE__, __TIME__);
	return 0;
}

struct atcmd_parser_t  atcmd_parser_tbl[] =
{
    { "SDB"  ,   atcmd_parser_sdb_proc },
    { "&REGR",   atcmd_parser_regr_proc },
    { "&REGW",   atcmd_parser_regw_proc },
   	{ "&FLSW",   atcmd_parser_flsw_proc},
   	{ "&FLSR",   atcmd_parser_flsr_proc},
   	{ "Z",       atcmd_parser_reset_proc},
    { "WBGR",    atcmd_parser_wbgr_proc},
    { "QVER",    atcmd_parser_qver_proc},
    { NULL,      NULL },
};