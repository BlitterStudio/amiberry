
UAE 680x0 CPU Tester

I finally wrote utility (This was my "Summer 2019" project) that can be used to verify operation of for example software emulated or FPGA 680x0 CPUs.
Tester is based on UAE CPU core (gencpu generated special test core). All the CPU logic comes from UAE CPU core.

Verifies:

- All CPU registers (D0-D7/A0-A7, PC and SR/CCR)
- All FPU registers (FP0-FP7, FPIAR, FPCR, FPSR)
- Generated exception and stack frame contents (if any)
- Memory writes, including stack modifications (if any)
- Loop mode for JIT testing. (generates <test instruction>, store CCR state, dbf dn,loop)
- Supports 68000, 68010, 68020, 68030 (only difference between 020 and 030 seems to be data cache and MMU), 68040 and 68060.
- Cycle counts (68000/68010, Amiga only)

Tests executed for each tested instruction:

- Every CCR combination or optionally only all zeros, all ones CCR (32 or 2 tests)
- Every FPU condition combination (4 bits) + 2 precision bits + 2 rounding bits (256 tests)
- Every addressing mode, including optionally 68020+ addressing modes.
- If instruction generated privilege violation exception, extra test round is run in supervisor mode.
- Optionally can do any combination of T0, T1, S and M -bit SR register extra test rounds.
- Every opcode value is tested. Total number of tests per opcode depends on available addressing modes etc. It can be hundreds of thousands or even millions..
- Optionally can be used to fully validate address and bus errors. Bus error testing requires extra hardware/logic.

Test generation details:

If normal mode: instruction's effective address is randomized. It is accepted if it points to any of 3 test memory regions. If it points outside of test memory, it will be re-randomized few times. Test will be skipped if current EA makes it impossible to point to any of 3 test regions.
If 68000/68010 and address error testing is enabled: 2 extra test rounds are generated, one with even and another with odd EAs to test and verify address errors.

If target EA mode: instruction's effective address always points to configured target address(es). Very useful when testing address or bus errors.

Notes and limitations:

- Test generator was originally very brute force based, it should be more intelligent.. Now has optional target src/dst/opcode modes for better bus/address error testing.
- Bus and address error testing is optional, if disabled, generated tests never cause bus/address errors.
- RTE test only tests stack frame types 0 and 2 (if 68020+)
- All tests that would halt or reset the CPU are skipped (RESET in supervisor mode, STOP parameter that would stop the CPU etc)
- Single instruction test set will take long time to run on real 68000. Few minutes to much longer...
- Undefined flags (for example DIV and CHK or 68000/010 bus address error) are also verified. It probably would be good idea to optionally filter them out.
- TAS test will return wrong results if test RAM region is not fully TAS read-modify-write special memory access compatible.
- if 24-bit address space and high ram is enabled, tester can generate word or long accesses that wrap around. (For example: move.l $fffffe,d0)

Tester compatibility (integer instructions only):

68000: Complete. Including bus and address error stack frame/register/CCR modification undocumented behavior. Full cycle count support.
68010: Almost complete (same as 68000). Loop mode is also fully supported. NOTE: DIVS overflow undocumented N-flag is not fully correct.
68020: Almost complete (DIV undocumented behavior is not yet known)
68030: Same as 68020.
68040: Almost complete (Weird unaligned MOVE16 behavior which may be board specific).
68060: Same as 68040.

68000/68010 cycle count testing:

Cycle counting requires 100% accurate timing also for following instructions:
- BSR.B
- NOP
- MOVE.W ABS.L,ABS.L
- MOVE SR,-(SP)
- MOVE.W #x,ABS.L (68010 only)
- RTE
- Illegal instruction exception
- If instruction internally generates exception, internal exception also needs to be cycle-accurate.

0xDFF006 is used for cycle counting = accuracy will be +-2 CPU cycles. 0xDFF006 behavior must be accurate.
Currently only supported hardware for cycle counting is 7MHz 68000/68010 PAL Amiga with real Fast RAM.

FPU testing:

68040/060 FPU test hardware must not have any 68040/060 or MMU support libraries loaded.


--

Not implemented or only partially implemented:

68010+:

- MOVEC: Most control registers tested: Write all ones, write all zeros, read it back, restore original value. 68040+ TC/TT registers enable bit is always zero.
- RTE: address/bus error undefined fields are skipped. Basic frame types 0, 2 and 8 are verified, also unsupported frame types are tested, error is reported if CPU does not generate frame exception.
- 68020+ undefined addressing mode bit combinations are not tested.

All models:

- STOP test only checks immediate values that do not stop the CPU.
- MMU instructions (Not going to happen)
- 68020+ cache related instructions.
- FPU FSAVE/FRESTORE, FPU support also isn't fully implemented yet.

Build instructions:

- buildm68k first (already built if UAE core was previously compiled)
- gencpu with CPU_TEST=1 define. This creates cpuemu_x_test.cpp files (x=90-94), cpustbl_test.cpp and cputbl_test.h
- build cputestgen project.
- build native Amiga project (cputest directory). Assembly files probably only compile with Bebbo's GCC.

More CPU details in WinUAE changelog.

--

Test generator quick instructions:

Update cputestgen.ini to match your CPU model, memory settings etc. Enable (enabled=1) preset test set, for example 68000 Basic test is good starting point.

"Low memory" = memory accessible using absolute word addressing mode, positive value (0x0000 to 0x7fff). Can be larger.
"High memory" = memory accessible using absolute word addressing mode, negative value (0xFFF8000 to 0xFFFFFFFF)

If high memory is ROM space (like on 24-bit address space Amigas), memory region is used for read only tests, use "high_rom=<path to rom image>" to select ROM image. Last 32k of image is loaded.

Use "test_low_memory_start"/"test_high_memory_start" and "test_low_memory_end"/"test_high_memory_end" to restrict range of memory region used for tests, for example if part of region is normally inaccessible.

"test_memory_start"/"test_memory_size" is the main test memory, tested instruction and stack is located here. Must be at least 128k but larger the size, the easier it is for the generator to find effective addresses that hit test memory. This memory space must be free on test target m68k hardware.

All 3 memory regions (if RAM) are filled with pseudo-random pattern and saved as "lmem.dat", "hmem.dat" and "tmem.dat"

Use feature_target_src_ea/feature_target_dst_ea=<one or more addresses separated by a comman> if you want generate test set that only uses listed addresses (of course instructions that can have memory source or destination EA are used). Useful for bus and address errors.

Usage of Amiga m68k native test program:

Copy all memory dat files, test executable compiled for target platform (currently only Amiga is supported) and data/<cpu model> contents to target system, keeping original directory structure.

cputest basic/all = run all tests, in alphabetical order. Stops when mismatch is detected. (Assuming 68000 Basic test set)
cputest basic/tst.b = run tst.b tests only
cputest basic/all tst.b = run tst.b, then tst.w and so on in alphabetical order until end or mismatch is detected.

If mismatch is detected, opcode word(s), instruction disassembly, registers before and after and reason message is shown on screen. If difference is in exception stack frame, both expected and returned stack frame is shown in hexadecimal.

--

Change log:

25.07.2020

- JIT loop mode test improved.

13.07.2020

- Added undefined_ccr option. Ignores undefined flags, currently supports DIVx, CHK and CHK2 (Instructions that don't have simple undefined behavior)

10.07.2020

- 68020+ stack frame PC field was ignored.
- JIT loop mode tester improved. CCR is now checked after each test round. Almost all instructions supported.
- File names are now 8.3 compatible.

31.05.2020

- 68010 bus error and byte size memory access: ignore bus error stack frame high byte of input and output buffers. Their contents depends on type of ALU operation, instruction type and probably more..
- If trace exception was generated after some other exception ("+EXC 9" in stack frame info line), it was not counted in final exception totals (Exx=1234)
- Replaced NOP with MOVEA.L A0,A0 in test code because 68040 NOP is considered branching instruction and triggers T0 trace.
- Interrupt test improved.

17.05.2020

- Rewritten disassembler to use indirect and validated opword reads, now it is safe to use when testing bus errors.
- Added 'B' branching identifier to register output. 0 = instruction didn't branch when test was genered, 1 = instruction branched when test was generated.

09.05.2020

- dat format changed, now it is possible to continue running test if previous test reported error.
- dat file stored FPU data uses less space.
- added forced_register option, force static content to any supported register.
- added -exit n command line parameter, n = number of errors until current test run gets aborted.
- added -fpuadj x,y,z command line parameter. x = max exponent difference allowed, y = ignore y last bits of mantissa, z = ignore z last zero bits of mantissa.
- 68000 bus/address error stack frame program counter field was ignored.

19.04.2020

- * = ignore rest of name, for example "cputest basic/neg*" will run NEG.B, NEG.W and NEG.L tests.
- FMOVECR, FMOVEM (includes FMOVE to/from control register), FDBcc, FBcc, FTRAPcc and FScc tests improved.

11.04.2020

- Working FPU support. Not all tests work correctly yet.
- Added FPU test presets.

15.03.2020

- Test coverage improved: If test uses data or address register as a source or destination, register gets modified after each test. Each register has different modification logic. (Some simply toggle few bits, some shift data etc..). Previously registers were static during single test round and start of each new round randomized register contents.
- 68020+ addressing mode test sets improved.

Only test generator was updated. Data structures or m68k tester has not been changed.

01.03.2020

- Added 68020+ test presets.
- CPU selection changed. CPU=680x0 line at the top of ini is now the main CPU selection field. Test is generated if CPU model matches preset's CPU and preset is active. This update allows use of same preset for multiple CPU models.
- cpu_address_space can be also used to select which CPU model is first 32-bit addressing capable model (normally 68020 or 68030).

16.02.2020

- 68000 prefetch bus error BTST Dn,#x, NBCD.B and LSLW fix. All prefetch bus error tests verified.
- 68000 read bus errors re-verified.

15.02.2020

- 68000 Address error timing fix (CHK.W cycle count error)
- 68000 MOVE to memory address error cycle order fixed.
- 68000 re-verified (except bus errors)

09.02.2020

- All 68000 tests are 100% confirmed, including full cycle-count support.

26.01.2020

- 68010 is now almost fully supported, including loop mode and cycle count verification.
- Removed most ColdFire instructions from disassembler. RTD was not disassembled if 68010. Added missing 68060 PCR and BUSCR control registers.
- Fixed ahist overflow when generating MOVEM with more than 1 SR flag combination.
- Some instructions (for example TRAP) had wrong expected cycle count if instruction generated any non-trace exception and also trace exception.
- added -skipexcccr parameter. Skip CCR check if instruction generates bus, address, divide by zero or CHK exception.
- added -skipmem (ignore memory write mismatches) -skipreg (ignore register mismatched) -skipccr (ignored CCR mismatch) parameters.

18.01.2020

- Cycle count validation (Amiga, 68000 only), including exceptions (except bus errors).
- Interrupt testing (Amiga only, INTREQ bits set one by one, validate correct exception).
- Multiple test sets can be generated and tested in single step.
- Stack usage reduced, gzip decompression works with default 4096 byte stack.

06.12.2021

- FPU testing improvements
