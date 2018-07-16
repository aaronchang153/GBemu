INCLUDE = -I include
CFLAGS  = -Wall -c
LFLAGS = -Wall -lmingw32 -lSDL2main -lSDL2
OBJECT_FILES = obj/main.o obj/cpu.o obj/memory.o obj/cartridge.o obj/timer.o obj/interrupt.o obj/graphics.o obj/display.o obj/joypad.o obj/audio.o obj/gameboy.o

GBemu: CFLAGS += -O2
#GBemu: LFLAGS += -Wl,-subsystem,windows

GBemu_Debug: INCLUDE += -I include/debug
GBemu_Debug: CFLAGS += -g -DDEBUG
GBemu_Debug: LFLAGS += -g -DDEBUG

GBemu : cpu.o memory.o cartridge.o timer.o interrupt.o graphics.o display.o joypad.o audio.o gameboy.o main.o
	gcc $(INCLUDE) $(OBJECT_FILES) $(LFLAGS) -o bin/GBemu.exe

GBemu_Debug : cpu.o memory.o cartridge.o timer.o interrupt.o graphics.o display.o joypad.o audio.o gameboy.o main.o
	gcc $(INCLUDE) $(CFLAGS) src/debug/disassemble.c -o obj/disassemble.o
	gcc $(INCLUDE) $(CFLAGS) src/debug/gbdebug.c -o obj/gbdebug.o
	gcc $(INCLUDE) $(OBJECT_FILES) obj/disassemble.o obj/gbdebug.o $(LFLAGS) -o bin/GBemu_Debug.exe

clean : 
	rm -f obj/*.o
	rm -f bin/GBemu_Debug.exe
	rm -f bin/GBemu.exe

### Individual module targets

cpu.o : src/cpu.c include/cpu.h
	gcc $(INCLUDE) $(CFLAGS) src/cpu.c -o obj/cpu.o

# decode unused
decode.o : src/decode.c include/decode.h
	gcc $(INCLUDE) $(CFLAGS) src/decode.c -o obj/decode.o

# opcode unused
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

audio.o : src/audio.c include/audio.h
	gcc $(INCLUDE) $(CFLAGS) src/audio.c -o obj/audio.o

gameboy.o : src/gameboy.c include/gameboy.h
	gcc $(INCLUDE) $(CFLAGS) src/gameboy.c -o obj/gameboy.o

main.o : main.c
	gcc $(INCLUDE) $(CFLAGS) main.c -o obj/main.o
