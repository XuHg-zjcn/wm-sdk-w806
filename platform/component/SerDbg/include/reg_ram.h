#ifndef RW_RAM_H
#define RW_RAM_H

#include "serdbg.h"

//待实现
SerDbg_Stat SDB_Read_Reg_op();
SerDbg_Stat SDB_Write_Reg_op();
SerDbg_Stat SDB_Read_Mem_op();
SerDbg_Stat SDB_Write_Mem_op();
SerDbg_Stat SDB_Read_Flash_op();
SerDbg_Stat SDB_Write_Flash_op();

#endif