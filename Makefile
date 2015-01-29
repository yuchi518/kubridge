CC=gcc
CFLAGS=
LDFLAGS=
SOURCES=kubridge_u.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=kubridge_u

obj-m += kubridge_k.o kubridge_k_tasks.o

all: user kernel

kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) CFLAGS=-D__KERNEL__=1 modules

user: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm kubridge_u
