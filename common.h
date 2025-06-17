#ifndef __COMMON_HEADER__
#define __COMMON_HEADER__

#define MAX_CMDS 50
#define MAX_CMD_LEN 128

typedef struct {
    char command[MAX_CMDS][MAX_CMD_LEN];
    int count;
} CommandList;

extern CommandList uboot_cmds;
extern CommandList linux_cmds;

#endif
