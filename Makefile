BASE_DIR := $(shell pwd)
CC      = gcc
INCLUDE = -I$(BASE_DIR)
SOURCE  = $(BASE_DIR)
EXEC    = autotty
CFLAGS  = -g $(INCLUDE) -Wall -pthread
RM		= rm -f

SRCS	:=  $(SOURCE)/main.c \
            $(SOURCE)/serial.c \
            $(SOURCE)/parser.c \

OBJS = $(SRCS:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(EXEC)
