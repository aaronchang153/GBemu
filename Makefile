INCLUDE = -I include
CFLAGS  = -Wall -g

all : GBemu

cpu.o : src/cpu.c include/cpu.h
	gcc $(INCLUDE) $(CFLAGS) -c src/cpu.c -o obj/cpu.o

memory.o : src/memory.c include/memory.h
	gcc $(INCLUDE) $(CFLAGS) -c src/memory.c -o obj/memory.o

GBemu : cpu.o memory.o