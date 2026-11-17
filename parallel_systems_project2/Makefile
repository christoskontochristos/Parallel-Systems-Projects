
CC = gcc

CFLAGS = -g -std=c11 -fopenmp -O3

SRC1 = exercise_1.c
SRC2 = exercise_2.c

EXE1 = exercise_1
EXE2 = exercise_2

all: $(EXE1) $(EXE2)

$(EXE1): $(SRC1)
	$(CC) $(CFLAGS) -o $(EXE1) $(SRC1)

$(EXE2): $(SRC2)
	$(CC) $(CFLAGS) -o $(EXE2) $(SRC2)

clean:
	rm -f $(EXE1) $(EXE2)
