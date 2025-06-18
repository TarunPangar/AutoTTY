#ifndef __COMMON_HEADER__
#define __COMMON_HEADER__

#include <pthread.h>
#include <sys/types.h>

#define MAX_CMDS 50
#define MAX_CMD_LEN 128

#define ENABLE_STDOUT_PRINTS	(0U)

typedef struct {
    char command[MAX_CMDS][MAX_CMD_LEN];
    int count;
} CommandList;

extern CommandList uboot_cmds;
extern CommandList linux_cmds;

typedef enum {
    STATE_WAIT,
    STATE_UBOOT,
    STATE_LINUX,
    MAX
} BootState;

extern pthread_mutex_t state_lock;
extern pthread_cond_t state_cond;
extern BootState current_state;

#endif
