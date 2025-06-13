#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <stdbool.h>

#include "serial.h"
#include "state_machine.h"

#define BUF_SIZE		2
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
		perror("tcgetattr");
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
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

void *read_serial_loop(void *arg)
{
	char buf[BUF_SIZE];
	int ret;

    while(1) {

        ret = read(tty_fd, buf, BUF_SIZE - 1);
        if (ret > 0) {
            buf[ret] = '\0';
            ret = write(log_fd, buf, ret);
            printf("%s", buf);

			// Accumulate for pattern detection
            if (accum_len + ret < MAX_ACCUM_BUF - 1) {
                memcpy(&accum_buf[accum_len], buf, ret);
                accum_len += ret;
                accum_buf[accum_len] = '\0';
            } else {
                accum_len = 0;
                accum_buf[0] = '\0';
            }

			// Detect boot stages
			pthread_mutex_lock(&state_lock);
			if (strstr(accum_buf, "Hit any key to stop autoboot:")) {
                printf("Detected autoboot prompt! Sending key to interrupt...\n");
                write(tty_fd, "\n", 1);		// Send Enter key to interrupt autoboot
                accum_len = 0;
			} else if (strstr(accum_buf, "j722s-evm login:")) {
                printf("Detected linux login prompt! Sending key to login...\n");
                write(tty_fd, "root\n", 5);		// Send Enter key to interrupt autoboot
                accum_len = 0;
            } else if (strstr(accum_buf, "=>")) {
                printf("Detected U-Boot prompt!\n");
                current_state = STATE_UBOOT;
                pthread_cond_signal(&state_cond);
            } else if (strstr(accum_buf, "root@j722s-evm:")) {	// || strstr(accum_buf, "# ")) {
                printf("Detected Linux shell prompt!\n");
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
		}

		if (current_state == STATE_UBOOT) {
			printf("[writer] Sending u-boot commands...\n");
			write(tty_fd, "sf probe\n", 9);
			sleep(1);
			write(tty_fd, "boot\n", 5);
			current_state = STATE_WAIT;
		} else if(current_state == STATE_LINUX) {
			printf("[writer] Sending linux commands...\n");
			write(tty_fd, "./run_script.sh\n", 17);
			current_state = STATE_WAIT;
		}
		pthread_mutex_unlock(&state_lock);
		sleep(1);
    }
	return NULL;
}
