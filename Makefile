CC=gcc
CFLAGS=-g

DEMO=mtsort

default: help

all: basic preemptive system

help:
	@echo "We can build in two ways (each of these will build $(DEMO).c):"
	@echo "\tmake basic - Build $(DEMO) with the basic ULT implementation."
	@echo "\tmake system - Build the $(DEMO) using the system pthreads implementation."

basic: $(DEMO).c mypthread.c mypthread.h cpu_info.c cpu_info.h
	$(CC) $(CFLAGS) $(DEMO).c -pthread mypthread.c cpu_info.c -o $(DEMO)-basic

system: $(DEMO).c
	$(CC) $(CFLAGS) $(DEMO).c -o $(DEMO)-system -pthread -include mypthread-system-override.h

mypthread_sample5: pthread_sample5.c mypthread.c mypthread.h cpu_info.c cpu_info.h
	$(CC) $(CFLAGS) pthread_sample5.c -pthread mypthread.c cpu_info.c -o $(DEMO)-basic

clean:
	@echo "Cleaning for ult..."
	@rm -f $(DEMO)-basic $(DEMO)-preemptive $(DEMO)-system
