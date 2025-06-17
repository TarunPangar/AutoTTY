#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "parser.h"
#include "common.h"

#define BUF_SIZE 1024

CommandList uboot_cmds = { .count = 0 };
CommandList linux_cmds = { .count = 0 };

int parse_cmd(char *line)
{
	if (line[0] == '>' && line[1] == ' ') {
		memmove(line, line + 2, strlen(line + 2) + 1);
		return 1;
	}
	return 0;
}

bool is_line_empty(const char *line)
{
	while (*line != '\0') {
		if (!isspace((unsigned char)*line)) {
			return false;
		}
		line++;
	}
	return true;
}

int read_line(int fd, char *line)
{
	ssize_t bytes_read;
	char ch;
	int line_index = 0;

	memset(line, 0, BUF_SIZE);

	while((bytes_read = read(fd, &ch, 1)) > 0) {
		if (ch == '\n') {
			break;
		}
		if (line_index < BUF_SIZE - 1) {
			line[line_index++] = ch;
		}
	}
	line[line_index] = '\0';
	return (bytes_read > 0 || line_index > 0) ? 1 : 0;
}

int parse_cfg(int fd)
{
	/* Initializations */
	char line[BUF_SIZE];
	int len;
	bool flag = false;

	while ((len = read_line(fd, line)) > 0) {
		if (is_line_empty(line)) {
			continue;	// Skip blank lines
		}

		if (strncmp(line, "[u-boot]", 8) == 0) {
			flag = true;
			continue;
		} else if (strncmp(line, "[linux]", 7) == 0) {
			flag = false;
			continue;
		}
		
		if (line[0] == '>') {
			parse_cmd(line);
			if (flag && uboot_cmds.count < MAX_CMDS) {
				// update u-boot structure
				strncpy(uboot_cmds.command[uboot_cmds.count++], line, MAX_CMD_LEN);
			} else if (!flag && linux_cmds.count < MAX_CMDS) {
				// update linux structure
				strncpy(linux_cmds.command[linux_cmds.count++], line, MAX_CMD_LEN);
			}
		}
	}

	printf("U-Boot Commands:\n");
	printf("u-boot count  %d\n", uboot_cmds.count);
	for (int i = 0; i < uboot_cmds.count; i++) {
		printf("  %s\n", uboot_cmds.command[i]);
	}

	printf("Linux Commands:\n");
	printf("linux count  %d\n", linux_cmds.count);
	for (int i = 0; i < linux_cmds.count; i++) {
		printf("  %s\n", linux_cmds.command[i]);
	}

	return 0;
}
