
NOWDATE := "\"$(shell date "+%-d.%-m.%Y")\""
NOWTIME := "\"$(shell date "+%T")\""

CC=/opt/amiga/bin/m68k-amigaos-gcc
AS=/opt/amiga/bin/m68k-amigaos-as

CFLAGS = -mcrt=nix13 -O2 -m68000 -fomit-frame-pointer -msoft-float -DREVDATE=$(NOWDATE) -DREVTIME=$(NOWTIME) -DAMIGA -DM68K
LINK_CFLAGS = -mcrt=nix13 -lm -s

OBJS = main.o dir.o asm040.o asm060.o amiga.o \
	decode_ea.o globals.o opcode_handler_cpu.o opcode_handler_fpu.o \
	opcode_handler_mmu.o opcodes_cpu.o opcodes_fpu.o opcodes_mmu.o util.o \
	inflate.o

all: $(OBJS)
	$(CC) $(LINK_CFLAGS) -o cputest $^

main.o: main.c
	$(CC) $(CFLAGS) -I. -c -o $@ main.c

dir.o: dir.c
	$(CC) $(CFLAGS) -I. -c -o $@ dir.c

decode_ea.o: adis/decode_ea.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/decode_ea.c

globals.o: adis/globals.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/globals.c

opcode_handler_cpu.o: adis/opcode_handler_cpu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcode_handler_cpu.c

opcode_handler_fpu.o: adis/opcode_handler_fpu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcode_handler_fpu.c

opcode_handler_mmu.o: adis/opcode_handler_mmu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcode_handler_mmu.c

opcodes_cpu.o: adis/opcodes_cpu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcodes_cpu.c

opcodes_fpu.o: adis/opcodes_fpu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcodes_fpu.c

opcodes_mmu.o: adis/opcodes_mmu.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/opcodes_mmu.c

util.o: adis/util.c
	$(CC) $(CFLAGS) -I. -c -o $@ adis/util.c

inflate.o: inflate.S
	$(CC) $(CFLAGS) -I. -c -o $@ inflate.S

asm040.o: asm040.S
	$(AS) -m68020  -o $@ asm040.S

asm060.o: asm060.S
	$(AS) -m68020  -o $@ asm060.S

amiga.o: amiga.S asm.S
	$(AS) -m68020  -o $@ amiga.S
