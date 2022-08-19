/**************************************************************************
 * Copyright 2022 Xu Ruijun
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
#ifndef ATCMD_PROC_H
#define ATCMD_PROC_H

#include "wm_type_def.h"

typedef enum{
  atcmd_noerr = 0,
  atcmd_err_maxargs = 1,
  atcmd_err_maxnamelen = 2,
  atcmd_err_notfound = 2,
  atcmd_err_param = 3,
}atcmd_stat;


#define ATCMD_PROC_MAX_ARGC     10
#define ATCMD_PROC_NAME_MAX_LEN 10


typedef struct{
    char      *name;
    int       (*proc_func)(int argc, char* argv[], char op);
}atcmd_proc_t;


#endif
