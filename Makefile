# #####################################################################
# Important
# see /usr/share/arduino/harware/arduino/board.txt for information
# #####################################################################

#make upload to upload
#make to compile
#make size for your information

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

PORT = usb
TARGET = $(BINDIR)/foo
SRC = $(wildcard $(SRCDIR)/*.c)
CXXSRC = $(wildcard $(SRCDIR)/*.cpp)
ASCRC =  $(wildcard $(SRCDIR)/*.S)
MCU = attiny85

#cpu speed
F_CPU = 8000000
#intel hex format
FORMAT = ihex
UPLOAD_RATE = 8192

# Name of this Makefile (used for "make depend").
MAKEFILE = Makefile

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs

# Optimization options : from 0 to 3 with s and fast
OPT = s

# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU)
CXXDEFS = -DF_CPU=$(F_CPU)
#ASDEFS = -DF_CPU=$(F_CPU)

# Place -I options here
CINCS = -I$(INCDIR)
CXXINCS = -I$(INCDIR)
ASINCS = -I$(INCDIR)


# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=c99
CDEBUG = -g$(DEBUG)
CWARN = -Wall -Wstrict-prototypes -Wextra
CTUNING = -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fdata-sections -ffunction-sections
CEXTRA= -mrelax
#CEXTRA = -Wa,-adhlns=$(<:.c=.lst)

CFLAGS = $(CDEBUG) $(CDEFS) $(CINCS) -O$(OPT) $(CWARN) $(CSTANDARD) $(CEXTRA) $(CTUNING)
CXXFLAGS = $(CDEFS) $(CINCS) -O$(OPT) $(CDEBUG) $(CWARN) $(CEXTRA) $(CTUNING)
#ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs $(ASINCS)
LDFLAGS = -Wl,-gc-sections


# Programming support using avrdude. Settings and variables.
#AVRDUDE_PROGRAMMER = avr109
AVRDUDE_PROGRAMMER = usbasp
AVRDUDE_PORT = $(PORT)
#AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex -U eeprom:w:$(TARGET).eep
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_WRITE_FLASH += -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
AVRDUDE_FLAGS = -F -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) \
  -b $(UPLOAD_RATE)

# Program settings
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -fv
MV = mv -fv
SED = sed

# Define all object files.
OBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o) $(CXXSRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) $(ASRC:$(SRCDIR)/%.S=$(OBJDIR)/%.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(CXXSRC:.cpp=.lst) $(SRC:.c=.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_CXXFLAGS = -mmcu=$(MCU) -I. $(CXXFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)

# Default target.
all: .depend build size

build: elf hex eep .depend

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym

# Program the device.
upload: $(TARGET).hex $(TARGET).eep $(TARGET).elf
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)

# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
--change-section-address .data-0x800000 \
--change-section-address .bss-0x800000 \
--change-section-address .noinit-0x800000 \
--change-section-address .eeprom-0x810000

coff: $(TARGET).elf
	$(COFFCONVERT) -O coff-avr $(TARGET).elf $(TARGET).cof

extcoff: $(TARGET).elf
	$(COFFCONVERT) -O coff-ext-avr $(TARGET).elf $(TARGET).cof

.SUFFIXES: .elf .hex .eep .lss .sym

.elf.hex:
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.eep:
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
	$(NM) -n $< > $@

# Link: create ELF output file from object files.
$(TARGET).elf: $(OBJ)
	$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)

# Compile: create object files from C++ source files.
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $(ALL_CXXFLAGS) $< -o $@

# Compile: create object files from C source files.
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@

# Compile: create assembler files from C source files.
$(OBJDIR)/%.s: $(SCRDIR)/%.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@

# Assemble: create object files from assembler source files.
$(OBJDIR)/%.o: $(SRCDIR)/%.S
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

size: $(TARGET).elf
	@$(SIZE) --mcu=$(MCU) --format=avr $<

# Target: clean project.
clean:
	$(REMOVE) $(TARGET).hex $(TARGET).eep $(TARGET).cof $(TARGET).elf \
	$(TARGET).map $(TARGET).sym $(TARGET).lss \
	$(OBJ) $(LST) $(SRC:.c=.s) $(SRC:.c=.d) $(CXXSRC:.cpp=.s) $(CXXSRC:.cpp=.d)
	$(REMOVE) .depend

.depend:
	$(CC) -MM -I$(INCDIR) $(SRC) $(ALL_CFLAGS) > $@
	$(SED) -i 's/\(.*\.o:\)/$(OBJDIR)\/\1/g' $@

# Including dependencies
-include .depend

.PHONY:	all build elf hex eep lss sym program coff extcoff clean size
