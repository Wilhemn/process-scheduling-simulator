CC = gcc
CCOPTS = -Wall -c -g -ggdb
LINKOPTS = -Wall -g -ggdb -pthread

EXEC=scheduler
OBJECTS=scheduler.o schedulerSimulation.o list.o tests.o sched_threads.o

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LINKOPTS) -o $@ $^

%.o:%.c
	$(CC) $(CCOPTS) -o $@ $^

test: scheduler
	- ./scheduler -fifo 5 10 test
	- ./scheduler -rr 5 10 test

clean:
	- $(RM) $(EXEC)
	- $(RM) $(OBJECTS)
	- $(RM) *~
