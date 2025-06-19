#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <stdbool.h>

#include "serial.h"
#include "common.h"

#define MAX_ACCUM_BUF	1024

extern int tty_fd;
extern int log_fd;
extern volatile bool running;

static char accum_buf[MAX_ACCUM_BUF];
static int accum_len = 0;

int init_serial(int fd)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
#if (ENABLE_STDOUT_PRINTS == 1U)
		perror("tcgetattr");
#endif
		return -1;
	}

	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tty.c_oflag &= ~OPOST;

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
#if (ENABLE_STDOUT_PRINTS == 1U)
		perror("tcsetattr");
#endif
		return -1;
	}

	return 0;
}

void *read_serial_loop(void *arg)
{
	char ch;
	int ret;
	int uboot_cnt = 0;
	char prev_char = 0;

    while(1) {

        ret = read(tty_fd, &ch, 1);
        if (ret > 0) {
			if (prev_char == '\n' && ch == '\n') {
				continue;
			}
			prev_char = ch;
            ret = write(log_fd, &ch, ret);
            printf("%c", ch);

			// Accumulate for pattern detection
            if (accum_len + ret < MAX_ACCUM_BUF - 1) {
				accum_buf[accum_len] = ch;
                accum_len += ret;
                accum_buf[accum_len] = '\0';
            } else {
                accum_len = 0;
                accum_buf[0] = '\0';
            }

			// Detect boot stages
			pthread_mutex_lock(&state_lock);
			if (strstr(accum_buf, "Hit any key to stop autoboot:")) {
#if (ENABLE_STDOUT_PRINTS == 1U)
                printf("Detected autoboot prompt! Sending key to interrupt...\n");
#endif
                write(tty_fd, "\n", 1);		// Send Enter key to interrupt autoboot
                accum_len = 0;
			} else if (strstr(accum_buf, "j722s-evm login:")) {
#if (ENABLE_STDOUT_PRINTS == 1U)
                printf("Detected linux login prompt! Sending key to login...\n");
#endif
                write(tty_fd, "root\n", 5);		// Send Enter key to interrupt autoboot
                accum_len = 0;
            } else if (strstr(accum_buf, "=>") && uboot_cnt < 1) {
#if (ENABLE_STDOUT_PRINTS == 1U)
                printf("Detected U-Boot prompt!\n");
#endif
				uboot_cnt++;
                current_state = STATE_UBOOT;
                pthread_cond_signal(&state_cond);
            } else if (strstr(accum_buf, "root@j722s-evm:")) {	// || strstr(accum_buf, "# ")) {
#if (ENABLE_STDOUT_PRINTS == 1U)
                printf("Detected Linux shell prompt!\n");
#endif
                current_state = STATE_LINUX;
                pthread_cond_signal(&state_cond);
            }
			pthread_mutex_unlock(&state_lock);
        }
    }
	return NULL;
}

void *write_serial_loop(void *args)
{
    while (running) {
		pthread_mutex_lock(&state_lock);
		while (current_state == STATE_WAIT && running) {
			pthread_cond_wait(&state_cond, &state_lock);
		}

		if (!running) {
			pthread_mutex_unlock(&state_lock);
			break;
		}

		BootState state = current_state;
		current_state = STATE_WAIT;
		pthread_mutex_unlock(&state_lock);

		if (state == STATE_UBOOT) {
#if (ENABLE_STDOUT_PRINTS == 1U)
			printf("[writer] Sending u-boot commands...\n");
#endif
			for(int i = 0; i < uboot_cmds.count && running; i++) {
				dprintf(tty_fd, "%s\n", uboot_cmds.command[i]);
#if (ENABLE_STDOUT_PRINTS == 1U)
				printf("[U-Boot] Sent: %s\n", uboot_cmds.command[i]);
#endif
				sleep(1);
			}
		} else if(state == STATE_LINUX) {
#if (ENABLE_STDOUT_PRINTS == 1U)
			printf("[writer] Sending linux commands...\n");
#endif
			for(int i = 0; i < linux_cmds.count && running; i++) {
				dprintf(tty_fd, "%s\n", linux_cmds.command[i]);
#if (ENABLE_STDOUT_PRINTS == 1U)
				printf("[linux] Sent: %s\n", linux_cmds.command[i]);
#endif
				sleep(1);
            }
		}
	}
	return NULL;
}
