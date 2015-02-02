CC=gcc
CFLAGS=-g
LDFLAGS=
SOURCES=kubridge_u.c kubridge_u_tasks.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=kubridge_u

obj-m += kubridge_k.o kubridge_k_tasks0.o kubridge_k_tasks1.o kubridge_k_tasks2.o kubridge_k_tasks3.o

all: user kernel

kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) CFLAGS=-D__KERNEL__=1 modules

user: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -pthread -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm kubridge_u
