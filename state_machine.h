#ifndef __STATE_MACHINE_HEADER__
#define __STATE_MACHINE_HEADER__

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
