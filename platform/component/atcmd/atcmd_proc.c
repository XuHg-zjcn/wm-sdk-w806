#include "atcmd_proc.h"
#include <stdio.h>
#include "wm_hal.h"
#include "reg_ram.h"


extern SDB_RegSave serdbg_regsave;

const char HwVer[] = "H0.312";
const char FirmWareVer[] = "0.01";

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
    //TODO: 修改AT指令解析器，不要读取剩余缓冲区，退出中断。
    //这一才能保证下一个字符一定能被读取
    serdbg_regsave.stat = SDB_WaitCap;
    //下一次串口中断时保存寄存器，并进入serdbg_paser_cmd()函数
    //详细请见"uart0_irqhandler.S"
}

static int atcmd_parser_reset_proc(struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len)
{
    CLEAR_REG(RCC->RST);                     // reset all peripherals
    uint32_t rv = *(uint32_t*)(0x00000000U); // get reset vector
    ((void (*)())(rv))();                    // go to ROM
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
    { "SDB",     atcmd_parser_sdb_proc},
    { "Z",       atcmd_parser_reset_proc},
    { "WBGR",    atcmd_parser_wbgr_proc},
    { "QVER",    atcmd_parser_qver_proc},
    { NULL,      NULL },
};
