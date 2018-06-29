INCLUDE = -I include -I include/debug -I SDL2/include/SDL2 -L SDL2/lib
CFLAGS  = -Wall -g -c -DDEBUG
LFLAGS = -Wall -g -lmingw32 -lSDL2main -lSDL2# -Wl,-subsystem,windows
OBJECT_FILES = obj/main.o obj/cpu.o obj/decode.o obj/opcode.o obj/memory.o obj/cartridge.o obj/timer.o obj/interrupt.o obj/graphics.o obj/display.o obj/joypad.o obj/gameboy.o

all : GBemu

cpu.o : src/cpu.c include/cpu.h
	gcc $(INCLUDE) $(CFLAGS) src/cpu.c -o obj/cpu.o

decode.o : src/decode.c include/decode.h
	gcc $(INCLUDE) $(CFLAGS) src/decode.c -o obj/decode.o

opcode.o : src/opcode.c include/opcode.h
	gcc $(INCLUDE) $(CFLAGS) src/opcode.c -o obj/opcode.o

memory.o : src/memory.c include/memory.h
	gcc $(INCLUDE) $(CFLAGS) src/memory.c -o obj/memory.o

cartridge.o : src/cartridge.c include/cartridge.h
	gcc $(INCLUDE) $(CFLAGS) src/cartridge.c -o obj/cartridge.o

timer.o : src/timer.c include/timer.h
	gcc $(INCLUDE) $(CFLAGS) src/timer.c -o obj/timer.o

interrupt.o : src/interrupt.c include/interrupt.h
	gcc $(INCLUDE) $(CFLAGS) src/interrupt.c -o obj/interrupt.o

graphics.o : src/graphics.c include/graphics.h
	gcc $(INCLUDE) $(CFLAGS) src/graphics.c -o obj/graphics.o

display.o : src/display.c include/display.h
	gcc $(INCLUDE) $(CFLAGS) src/display.c -o obj/display.o

joypad.o : src/joypad.c include/joypad.h
	gcc $(INCLUDE) $(CFLAGS) src/joypad.c -o obj/joypad.o

gameboy.o : src/gameboy.c include/gameboy.h
	gcc $(INCLUDE) $(CFLAGS) src/gameboy.c -o obj/gameboy.o

main.o : main.c
	gcc $(INCLUDE) $(CFLAGS) main.c -o obj/main.o

GBemu_Debug : cpu.o opcode.o decode.o memory.o cartridge.o timer.o interrupt.o graphics.o display.o joypad.o gameboy.o main.o
	gcc $(INCLUDE) $(CFLAGS) src/debug/disassemble.c -o obj/disassemble.o
	gcc $(INCLUDE) $(CFLAGS) src/debug/gbdebug.c -o obj/gbdebug.o
	gcc $(INCLUDE) $(OBJECT_FILES) obj/disassemble.o obj/gbdebug.o $(LFLAGS) -o bin/GBemu_Debug.exe

clean : 
	rm -f obj/*.o
	rm -f bin/GBemu_Debug.exe