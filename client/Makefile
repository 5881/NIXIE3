SDK=/opt/esp-open-sdk/ESP8266_NONOS_SDK-2.1.0-18-g61248df
DRIVER=$(SDK)/driver_lib
CC=xtensa-lx106-elf-gcc
SIZE=xtensa-lx106-elf-size
ESPTOOL=esptool.py
INC  = -I./inc
INC += -I$(DRIVER)/include
CFLAGS= -std=c99  -O0  -Wall -Wno-unused-variable -mlongcalls -DICACHE_FLASH $(INC)
LDFLAGS= -L$(SDK)/lib  -T$(SDK)/ld/eagle.app.v6.ld
LDLIBS=-nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc
OBJ=main.o uart.o spilib.o
TARGET=connect
.PHONY: all clean

%.o:	$(DRIVER)/driver/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<
all:	$(OBJ)
	$(CC) $(LDFLAGS) -o $(TARGET).elf  $(OBJ) $(LDLIBS)
	$(SIZE)  $(TARGET).elf
	$(ESPTOOL) elf2image $(TARGET).elf
flash:
	$(ESPTOOL) -b 3000000 write_flash 0 $(TARGET).elf-0x00000.bin 0x10000 $(TARGET).elf-0x10000.bin
clean:
	@rm -v $(TARGET).elf $(OBJ) $(TARGET).elf-0x?0000.bin
