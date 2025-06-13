#ifndef __SERIAL_HEADER__
#define __SERIAL_HEADER__

int init_serial(int fd);
void *read_serial_loop(void *arg);
void *write_serial_loop(void *arg);

#endif
