#ifndef PROC_ATCMD_H
#define PROC_ATCMD_H

#include "wm_type_def.h"

#define ATCMD_PARSER_ERR_OK              0
#define ATCMD_PARSER_ERR_INV_FMT         1
#define ATCMD_PARSER_ERR_UNSUPP          2
#define ATCMD_PARSER_ERR_OPS             3
#define ATCMD_PARSER_ERR_INV_PARAMS      4
#define ATCMD_PARSER_ERR_NOT_ALLOW       5
#define ATCMD_PARSER_ERR_MEM             6
#define ATCMD_PARSER_ERR_FLASH           7
#define ATCMD_PARSER_ERR_BUSY            8
#define ATCMD_PARSER_ERR_SLEEP           9
#define ATCMD_PARSER_ERR_JOIN            10
#define ATCMD_PARSER_ERR_NO_SKT          11
#define ATCMD_PARSER_ERR_INV_SKT         12
#define ATCMD_PARSER_ERR_SKT_CONN        13
#define ATCMD_PARSER_ERR_UNDEFINE        64
#define ATCMD_PARSER_ERR_SCANNING		14

typedef unsigned int u32;
typedef unsigned char u8;

#define ATCMD_PARSER_MAX_ARG      10
#define ATCMD_PARSER_NAME_MAX_LEN 10

struct atcmd_parser_token_t
{
    char   name[ATCMD_PARSER_NAME_MAX_LEN];
    u32   op;
    char  *arg[ATCMD_PARSER_MAX_ARG];
    u32   arg_found;
};

typedef int (* atcmd_proc)(struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len);

struct atcmd_parser_t
{
    char   *name;
    int (* proc_func)(struct atcmd_parser_token_t *tok, char *res_resp, u32 *res_len);
};

int hexstr_to_uinit(char *buf, u32 *d);
int atcmd_parser_ok_resp(char *buf);
int atcmd_parser_err_resp(char *buf, int err_code);

#endif