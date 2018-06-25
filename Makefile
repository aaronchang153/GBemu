INCLUDE = -I include -I include/debug
CFLAGS  = -Wall -g -DDEBUG
OBJECT_FILES = obj/main.o obj/cpu.o obj/decode.o obj/opcode.o obj/memory.o obj/timer.o obj/interrupt.o obj/graphics.o obj/gameboy.o

all : GBemu

cpu.o : src/cpu.c include/cpu.h
	gcc $(INCLUDE) $(CFLAGS) -c src/cpu.c -o obj/cpu.o

decode.o : src/decode.c include/decode.h
	gcc $(INCLUDE) $(CFLAGS) -c src/decode.c -o obj/decode.o

opcode.o : src/opcode.c include/opcode.h
	gcc $(INCLUDE) $(CFLAGS) -c src/opcode.c -o obj/opcode.o

memory.o : src/memory.c include/memory.h
	gcc $(INCLUDE) $(CFLAGS) -c src/memory.c -o obj/memory.o

timer.o : src/timer.c include/timer.h
	gcc $(INCLUDE) $(CFLAGS) -c src/timer.c -o obj/timer.o

interrupt.o : src/interrupt.c include/interrupt.h
	gcc $(INCLUDE) $(CFLAGS) -c src/interrupt.c -o obj/interrupt.o

graphics.o : src/graphics.c include/graphics.h
	gcc $(INCLUDE) $(CFLAGS) -c src/graphics.c -o obj/graphics.o

gameboy.o : src/gameboy.c include/gameboy.h
	gcc $(INCLUDE) $(CFLAGS) -c src/gameboy.c -o obj/gameboy.o

main.o : main.c
	gcc $(INCLUDE) $(CFLAGS) -c main.c -o obj/main.o

objects : cpu.o opcode.o decode.o memory.o timer.o interrupt.o graphics.o gameboy.o main.o

GBemu_Debug : cpu.o opcode.o decode.o memory.o timer.o interrupt.o graphics.o gameboy.o main.o
	gcc $(INCLUDE) $(CFLAGS) -c src/debug/disassemble.c -o obj/disassemble.o
	gcc $(INCLUDE) $(CFLAGS) -c src/debug/cpudebug.c -o obj/cpudebug.o
	gcc $(INCLUDE) $(CFLAGS) $(OBJECT_FILES) obj/disassemble.o obj/cpudebug.o -o bin/GBemu_Debug.exe

clean : 
	rm -f obj/*.o
	rm -f bin/GBemu_Debug.exe