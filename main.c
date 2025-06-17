#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include "serial.h"
#include "state_machine.h"
#include "parser.h"

#define TTY_PATH "/dev/ttyUSB2"
#define LOG_FILE "log_file.txt"
#define CFG_FILE "commands.cfg"

int tty_fd, log_fd, cfg_fd;
volatile bool running = true;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t state_cond = PTHREAD_COND_INITIALIZER;
BootState current_state = STATE_WAIT;

void handle_sigint(int signal)
{
	running = false;

	pthread_mutex_lock(&state_lock);
	pthread_cond_broadcast(&state_cond);	// Wake up any waiting threads
	pthread_mutex_unlock(&state_lock);

	close(tty_fd);
	close(log_fd);
	close(cfg_fd);
	printf("Shutting down gracefully....\n");
}

int main()
{
//	signal(SIGINT, handle_sigint);

	unlink(LOG_FILE);

	tty_fd = open(TTY_PATH, O_RDWR);
	if (tty_fd < 0) {
		perror("open tty");
		return 1;
	}

	log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (log_fd < 0) {
		perror("open log");
		return 1;
	}

	if (init_serial(tty_fd) < 0) {
		return 1;
	}

	cfg_fd = open(CFG_FILE, O_RDWR);
	if (cfg_fd < 0) {
        perror("open cfg");
        return 1;
    }

	parse_cfg(cfg_fd);

	pthread_t reader, writer;
	pthread_create(&reader, NULL, read_serial_loop, NULL);
	pthread_create(&writer, NULL, write_serial_loop, NULL);

	pthread_join(reader, NULL);
	pthread_join(writer, NULL);


	return 0;
}
