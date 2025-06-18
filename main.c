#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include "serial.h"
#include "parser.h"
#include "common.h"

#define TTY_PATH "/dev/ttyUSB2"
#define LOG_FILE "log_file.txt"
#define CFG_FILE "commands.cfg"

int tty_fd, log_fd, cfg_fd;
volatile bool running = true;
volatile sig_atomic_t signal_count = 0;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t state_cond = PTHREAD_COND_INITIALIZER;
BootState current_state = STATE_WAIT;

void handle_sigint(int signum)
{
	signal(SIGINT, handle_sigint);

	(void)signum;
	signal_count++;

	if (signal_count == 1) {
		const char *msg = "Are you trying to exit? Press Ctrl+C again to confirm.\n";
		write(STDOUT_FILENO, msg, strlen(msg));  // Safe alternative to printf
		return;
	} else if (signal_count == 2) {
		running = false;

		pthread_mutex_lock(&state_lock);
		pthread_cond_broadcast(&state_cond);	// Wake up any waiting threads
		pthread_mutex_unlock(&state_lock);

		close(tty_fd);
		close(log_fd);
		close(cfg_fd);
		printf("Shutting down gracefully....\n");
		exit(0);

	const char *shutdown_msg = "Shutting down gracefully....\n";
	write(STDOUT_FILENO, shutdown_msg, strlen(shutdown_msg));

	_exit(0);
	}
}

int main()
{
	signal(SIGINT, handle_sigint);

	unlink(LOG_FILE);

	tty_fd = open(TTY_PATH, O_RDWR);
	if (tty_fd < 0) {
#if (ENABLE_STDOUT_PRINTS == 1U)
		perror("open tty");
#endif
		return 1;
	}

	log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (log_fd < 0) {
#if (ENABLE_STDOUT_PRINTS == 1U)
		perror("open log");
#endif
		return 1;
	}

	if (init_serial(tty_fd) < 0) {
		return 1;
	}

	cfg_fd = open(CFG_FILE, O_RDWR);
	if (cfg_fd < 0) {
#if (ENABLE_STDOUT_PRINTS == 1U)
        perror("open cfg");
#endif
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
