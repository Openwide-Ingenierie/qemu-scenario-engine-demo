# Written by Victor Clément <victor.clement@openwide.fr>

# Qemu invocation options
QEMU=/home/victor/qemu-modif/bin/arm/arm-softmmu/qemu-system-arm
QEMU_MACHINE=versatilepb
QEMU_OPTIONS=-vga none \
			 -serial stdio

# Compilation parameters
CPU=arm926ej-s

# Files names
PROG_NAME=program
LD_SCRIPT=link.ld
OBJ=vectors.o irq_in.o


# THE Makefile

all: $(PROG_NAME).bin

run: all
	$(QEMU) \
	-M $(QEMU_MACHINE) \
	-kernel $(PROG_NAME).bin \
	$(QEMU_OPTIONS)

$(PROG_NAME).bin: $(PROG_NAME).elf
	arm-none-eabi-objcopy -O binary $(PROG_NAME).elf $(PROG_NAME).bin

$(PROG_NAME).elf: $(LD_SCRIPT) $(OBJ)
	arm-none-eabi-ld -T $^ -o $(PROG_NAME).elf

%.o: %.c
	arm-none-eabi-gcc -c -mcpu=arm926ej-s -g $< -o $@

%.o: %.s
	arm-none-eabi-as -mcpu=arm926ej-s -g $< -o $@

clean:
	rm *.o
	rm *.elf
	rm *.bin
	rm trace-*

