INCLUDE = -I include
CFLAGS  = -Wall -g
OBJECT_FILES = obj/main.o obj/cpu.o obj/decode.o obj/opcode.o obj/memory.o 

all : GBemu

cpu.o : src/cpu.c include/cpu.h
	gcc $(INCLUDE) $(CFLAGS) -c src/cpu.c -o obj/cpu.o

decode.o : src/decode.c include/decode.h
	gcc $(INCLUDE) $(CFLAGS) -c src/decode.c -o obj/decode.o

opcode.o : src/opcode.c include/opcode.h
	gcc $(INCLUDE) $(CFLAGS) -c src/opcode.c -o obj/opcode.o

memory.o : src/memory.c include/memory.h
	gcc $(INCLUDE) $(CFLAGS) -c src/memory.c -o obj/memory.o

main.o : main.c
	gcc $(INCLUDE) $(CFLAGS) -c main.c -o obj/main.o

GBemu : cpu.o opcode.o decode.o memory.o main.o
	gcc $(INCLUDE) $(CFLAGS) $(OBJECT_FILES) -o bin/GBemu.exe

clean : 
	rm -f obj/cpu.o
	rm -f obj/decode.o
	rm -f obj/opcode.o
	rm -f obj/memory.o
	rm -f obj/main.o
	rm -f bin/GBemu.exe