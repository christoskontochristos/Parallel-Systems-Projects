# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -lpthread #-std=c11

# Header files
HEADERS = my_rand.h timer.h

#source files and their corresponding executables
EXERCISE_SRCS = exercise1.c exercise2.c exercise3.c exercise4a.c exercise4b.c \
                 exercise5_a.c exercise5_b.c exercise5_c.c
MY_RAND_SRC = my_rand.c
EXERCISE_OBJS = $(EXERCISE_SRCS:.c=)

#default target: build all exercises
all: $(EXERCISE_OBJS)

#rule for building each exercise
%: %.c $(MY_RAND_SRC) $(HEADERS)
	$(CC) -o $@ $< $(MY_RAND_SRC) $(CFLAGS)

#clean target to remove all executables
clean:
	rm -f $(EXERCISE_OBJS)
