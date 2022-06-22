### User Configuration ###

# Name of ROM to build
TARGET := tex_sampling_demo

DEBUG ?= 0

ABI ?= 32

LTO ?= 0

# Do not change below this line!

# Default config generation
ifeq (,$(wildcard Makefile.config))
  DUMMY != printf "\# Python command, e.g. python3 (or location if not on the path)\nPYTHON := python3\n" > Makefile.config
  DUMMY != printf "\# UNFLoader location\nUNFLOADER := UNFLoader" >> Makefile.config
endif

PLATFORM := n64

# System config
include Makefile.config
SDK_INCLUDE := -I /etc/n64/usr/include
GCC_LINKER := -L $(N64_LIBGCCDIR)
#   ULTRA_LINKER := -L /etc/n64/lib
ifeq ($(ABI),n32)
  ULTRA_LINKER := -L platforms/n64/lib/n32 -L ../ultralib/
  LIBULTRA := gultra_rom_n32
  ABIFLAG := $(ABI)
else ifeq ($(ABI),eabi32)
  ULTRA_LINKER := -L platforms/n64/lib/eabi32 -L ../ultralib/
  LIBULTRA := gultra_rom_eabi32
  ABIFLAG := eabi
else ifeq ($(ABI),32)
    # ULTRA_LINKER := -L platforms/n64/lib/o32 -L ../ultralib/
    ULTRA_LINKER := -L /etc/n64/lib
    # LIBULTRA := gultra_rom
    LIBULTRA := ultra_rom
    ABIFLAG := $(ABI)
else
  $(error "Invalid abi $(ABI)")
endif

### Text variables ###

# These use the fact that += always adds a space to create a variable that is just a space
# Space has a single space, indent has 2
space :=
space +=

indent =
indent += 
indent += 

### Tools ###

# System tools
CD    := cd
CP    := cp
RM    := rm
CAT   := cat
GPERF := gperf


MKDIR := mkdir
MKDIR_OPTS := -p

RMDIR := rm
RMDIR_OPTS := -rf

PRINT := printf '
  ENDCOLOR := \033[0m
  WHITE     := \033[0m
  ENDWHITE  := $(ENDCOLOR)
  GREEN     := \033[0;32m
  ENDGREEN  := $(ENDCOLOR)
  BLUE      := \033[0;34m
  ENDBLUE   := $(ENDCOLOR)
  YELLOW    := \033[0;33m
  ENDYELLOW := $(ENDCOLOR)
ENDLINE := \n'

RUN := 

PREFIX  := mips-n64-

# Build tools
CC      := $(N64CHAIN)$(PREFIX)gcc
AS      := $(N64CHAIN)$(PREFIX)as
CPP     := $(N64CHAIN)$(PREFIX)cpp
CXX     := $(N64CHAIN)$(PREFIX)g++
LD      := $(N64CHAIN)$(PREFIX)g++
OBJCOPY := $(N64CHAIN)$(PREFIX)objcopy

CKSUM   := $(PYTHON) tools/n64cksum.py

ASSETPACK := tools/assetpack/assetpack
GLTF64    := tools/gltf64/gltf64
SOUNDCONV := tools/soundconv/soundconv

TOOLS := $(ASSETPACK) $(GLTF64) $(SOUNDCONV)

### Files and Directories ###

# Source files
PLATFORM_DIR := platforms/$(PLATFORM)
SRC_ROOT  := src
SRC_DIRS  := $(wildcard $(SRC_ROOT)/*) $(wildcard $(PLATFORM_DIR)/$(SRC_ROOT)/*)
C_SRCS    := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.c))
CXX_SRCS  := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.cpp))
ASM_SRCS  := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.s))
BIN_FILES := $(foreach src_dir,$(SRC_DIRS),$(wildcard $(src_dir)/*.bin))
LD_SCRIPT := n64.ld
BOOT      := $(PLATFORM_DIR)/boot/boot.6102
ENTRY_AS  := $(PLATFORM_DIR)/boot/entry.s

# Build root
ifeq ($(DEBUG),0)
BUILD_ROOT     := build/$(PLATFORM)/release
else
BUILD_ROOT     := build/$(PLATFORM)/debug
endif

# Asset files
ASSET_ROOT := assets

MODEL_DIR  := $(ASSET_ROOT)/models
MODELS     := $(wildcard $(MODEL_DIR)/*.gltf)
MODELS_OUT := $(addprefix $(BUILD_ROOT)/, $(MODELS:.gltf=))

IMAGES_DIR := $(ASSET_ROOT)/images
IMAGES     := $(wildcard $(IMAGES_DIR)/*.png)
IMAGES_OUT := $(addprefix $(BUILD_ROOT)/, $(IMAGES:.png=))

ASSETS_OUT  := $(MODELS_OUT)

ASSETS_DIRS  := $(MODEL_DIR)
ASSETS_BIN   := $(BUILD_ROOT)/assets.bin
ASSETS_OBJ   := $(ASSETS_BIN:.bin=.o)
# gperf input file
ASSETS_TXT   := $(ASSETS_BIN:.bin=.txt)
ASSETS_GPERF := $(ASSETS_BIN:.bin=_gperf.txt)
ASSETS_CPP   := $(ASSETS_BIN:.bin=_code.cpp)
ASSETS_CPP_O := $(ASSETS_CPP:.cpp=.o)

SAMPLES_DIR := $(ASSET_ROOT)/samples
SAMPLES     := $(wildcard $(SAMPLES_DIR)/*)
WAVETABLES  := $(BUILD_ROOT)/sounds.wbk
PTRBANK     := $(BUILD_ROOT)/sounds.ptr
FXBANK      := $(BUILD_ROOT)/fxbank.bin
WAVETABLES_O := $(WAVETABLES).o
PTRBANK_O    := $(PTRBANK).o
FXBANK_O     := $(FXBANK).o
SOUND_OBJS   := $(WAVETABLES_O) $(PTRBANK_O) $(FXBANK_O)

SFX_JSON := $(ASSET_ROOT)/sfx.json

# Build folders
BOOT_BUILD_DIR := $(BUILD_ROOT)/$(PLATFORM_DIR)/boot
BUILD_DIRS     := $(addprefix $(BUILD_ROOT)/,$(SRC_DIRS) $(ASSETS_DIRS)) $(BOOT_BUILD_DIR)

# Build files
C_OBJS   := $(addprefix $(BUILD_ROOT)/,$(C_SRCS:.c=.o))
CXX_OBJS := $(addprefix $(BUILD_ROOT)/,$(CXX_SRCS:.cpp=.o))
ASM_OBJS := $(addprefix $(BUILD_ROOT)/,$(ASM_SRCS:.s=.o))
BIN_OBJS := $(addprefix $(BUILD_ROOT)/,$(BIN_FILES:.bin=.o))
OBJS     := $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS) $(BIN_OBJS) $(ASSETS_CPP_O)
LD_CPP   := $(BUILD_ROOT)/$(LD_SCRIPT)
BOOT_OBJ := $(BUILD_ROOT)/$(BOOT).o
ENTRY_OBJ:= $(BUILD_ROOT)/$(ENTRY_AS:.s=.o)
D_FILES  := $(C_OBJS:.o=.d) $(CXX_OBJS:.o=.d) $(LD_CPP).d

CODESEG  := $(BUILD_ROOT)/codesegment.o
Z64      := $(addprefix $(BUILD_ROOT)/,$(TARGET).z64)
V64      := $(addprefix $(BUILD_ROOT)/,$(TARGET).v64)
ELF      := $(Z64:.z64=.elf)

### Flags ###

# Build tool flags

# TODO Add -fdata-sections to CFLAGS and CXXFLAGS
# Doing so will break alignas/alignment attributes, so arrays which require those need to be moved to a separate file
# compilied without those flags

ifeq ($(OS),Windows_NT)
CFLAGS     := -march=vr4300 -mtune=vr4300 -mfix4300 -mabi=32 -mno-shared -G 0 -mhard-float -fno-stack-protector -fno-common -fno-zero-initialized-in-bss \
			  -fno-PIC -mno-abicalls -fno-strict-aliasing -fno-inline-functions -ffreestanding -fwrapv \
			  -mno-check-zero-division -mno-split-addresses -mno-relax-pic-calls -mfp32 -mgp32 -mbranch-cost=1 \
			  -fno-dse -fno-check-pointer-bounds -Wno-chkp -mno-odd-spreg -fno-use-linker-plugin \
        -D_LANGUAGE_C -ffunction-sections
CXXFLAGS     := -march=vr4300 -mtune=vr4300 -mfix4300 -mabi=32 -mno-shared -G 0 -mhard-float -fno-stack-protector -fno-common -fno-zero-initialized-in-bss \
			  -fno-PIC -mno-abicalls -fno-strict-aliasing -fno-inline-functions -ffreestanding -fwrapv \
			  -mno-check-zero-division -mno-split-addresses -mno-relax-pic-calls -mfp32 -mgp32 -mbranch-cost=1 \
			  -fno-dse -fno-check-pointer-bounds -Wno-chkp -mno-odd-spreg -fno-use-linker-plugin \
        -fno-rtti -std=c++20 -D_LANGUAGE_C_PLUS_PLUS -ffunction-sections -fno-exceptions

else

ifeq ($(ABI),n32)
  MIPS_ARCH := 4300
  FORMAT := elf32-ntradbigmips
else
  MIPS_ARCH := 3000
  FORMAT := elf32-tradbigmips
endif
# CFLAGS     := -mabi=32 -ffreestanding -G 0 -D_LANGUAGE_C -ffunction-sections -fno-builtin-memset
# CXXFLAGS   := -mabi=32 -std=c++20 -fno-rtti -G 0 -D_LANGUAGE_C_PLUS_PLUS -ffunction-sections -fno-exceptions -fno-builtin-memset
CFLAGS     := -mips3 -march=vr4300 -mabi=$(ABIFLAG) -mfix4300 -mno-abicalls -fno-PIC -G 0 -fwrapv -ffunction-sections -fno-builtin-memset -fno-stack-protector -mno-check-zero-division -U_FORTIFY_SOURCE -D_LANGUAGE_C
CXXFLAGS   := -mips3 -march=vr4300 -mabi=$(ABIFLAG) -mfix4300 -mno-abicalls -fno-PIC -G 0 -fwrapv -ffunction-sections -fno-builtin-memset -fno-stack-protector -mno-check-zero-division -U_FORTIFY_SOURCE -D_LANGUAGE_C_PLUS_PLUS -std=c++20 -fno-rtti -fno-exceptions
endif
CPPFLAGS   := -I include -I $(PLATFORM_DIR)/include -I . -I src/ -Ilib/glm $(SDK_INCLUDE) -D_FINALROM -D_MIPS_SZLONG=32 -D_MIPS_SZINT=32 -D_ULTRA64 -D__EXTENSIONS__ -DF3DEX_GBI_2 -D_OS_LIBC_H_
# CPPFLAGS   := -I include -I $(PLATFORM_DIR)/include -I . -I src/ -Ilib/glm $(SDK_INCLUDE) -D_FINALROM -D_MIPS_SZLONG=32 -D_MIPS_SZINT=32 -D_ULTRA64 -D__EXTENSIONS__ -DF3DEX_GBI_2
WARNFLAGS  := -Wall -Wextra -Wpedantic -Wdouble-promotion -Wfloat-conversion
ASFLAGS    := -mtune=vr4300 -march=vr4300 -mabi=$(ABIFLAG) -mips3
LDFLAGS    := -march=vr4300 -mabi=$(ABIFLAG) -mips3 -T $(LD_CPP) -Wl,--accept-unknown-input-arch -Wl,--no-check-sections -Wl,-Map,$(BUILD_ROOT)/$(TARGET).map \
			  $(ULTRA_LINKER) -nostartfiles -Wl,-gc-sections -nostdlib $(ULTRA_LINKER) -L lib $(LIBS) -Wl,-gc-sections


ifeq ($(ABI),n32)
  LIBS := -ln_gmus_n32 -ln_gaudio_sc_n32 -l$(LIBULTRA)
else ifeq ($(ABI),eabi32)
  LIBS := -ln_gmus_eabi32 -ln_gaudio_sc_eabi32 -l$(LIBULTRA)
  CFLAGS   += -mgp32 -mfp32
  CXXFLAGS += -mgp32 -mfp32
  ASFLAGS  += -mgp32 -mfp32
  LDFLAGS  += -mgp32 -mfp32
else ifeq ($(ABI),32)
  # LIBS := -ln_gmus -ln_gaudio_sc -l$(LIBULTRA)
  LIBS := -ln_mus -ln_audio_sc -l$(LIBULTRA)
else
  $(error "Invalid abi $(ABI)")
endif

# LIBS := -l$(LIBULTRA)
SEG_LDFLAGS:= -march=vr4300 -mabi=$(ABIFLAG) -mips3 $(ULTRA_LINKER) -L lib $(LIBS) -e init -Wl,-gc-sections -u numberOfSetBits
LDCPPFLAGS := -P -Wno-trigraphs -DBUILD_ROOT=$(BUILD_ROOT) -Umips
OCOPYFLAGS := --pad-to=0x101000 --gap-fill=0xFF

ifeq ($(LTO),1)
  CFLAGS      += -flto
  CXXFLAGS    += -flto
  SEG_LDFLAGS += -flto
  LDFLAGS     += -flto
endif

OPT_FLAGS  := -Os -ffast-math

ifneq ($(DEBUG),0)
CPPFLAGS     += -DDEBUG_MODE
OPT_FLAGS    += -Ofast -freorder-blocks-algorithm=simple --param max-loop-header-insns=1
else
CPPFLAGS     += -DNDEBUG
endif


$(BUILD_ROOT)/platforms/n64/src/usb/usb.o: OPT_FLAGS := -O0
$(BUILD_ROOT)/platforms/n64/src/usb/usb.o: WARNFLAGS += -Wno-unused-variable -Wno-sign-compare -Wno-unused-function
$(BUILD_ROOT)/platforms/n64/src/usb/debug.o: WARNFLAGS += -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-double-promotion
$(ASSETS_CPP_O): WARNFLAGS += -Wno-missing-field-initializers -Wno-unused-parameter

### Rules ###

# Default target, all
all: $(Z64)

# Make directories
$(BUILD_ROOT) $(BUILD_DIRS) :
	@$(PRINT)$(GREEN)Creating directory: $(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(MKDIR) $@ $(MKDIR_OPTS)

# .cpp -> .o
$(BUILD_ROOT)/%.o : %.cpp | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C++ source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CXX) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)
	@mips-linux-gnu-strip $@ -N asdasdasads

# .cpp -> .o (build folder)
$(BUILD_ROOT)/%.o : $(BUILD_ROOT)/%.cpp | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C++ source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CXX) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)
	@mips-linux-gnu-strip $@ -N asdasdasads

# .c -> .o
$(BUILD_ROOT)/%.o : %.c | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling C source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(CC) $< -o $@ -c -MMD -MF $(@:.o=.d) $(CFLAGS) $(CPPFLAGS) $(OPT_FLAGS) $(WARNFLAGS)
	@mips-linux-gnu-strip $@ -N asdasdasads

# .bin -> .o
$(BUILD_ROOT)/%.o : %.bin | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) -B mips:$(MIPS_ARCH) $< $@
	@$(OBJCOPY) --rename-section .data=.bindata $@

# .bin -> .o (build folder)
$(BUILD_ROOT)/%.o : $(BUILD_ROOT)/%.bin | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying built binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) -B mips:$(MIPS_ARCH) $< $@
	@$(OBJCOPY) --rename-section .data=.bindata $@

# .s -> .o
$(BUILD_ROOT)/%.o : %.s | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Compiling ASM source file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(AS) $< -o $@ $(ASFLAGS)
	@mips-linux-gnu-strip $@ -N asdasdasads

# boot -> .o
$(BOOT_OBJ) : $(BOOT) | $(BOOT_BUILD_DIR)
	@$(PRINT)$(GREEN)Copying boot to object file$(ENDGREEN)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) $< $@
	@mips-linux-gnu-strip $@ -N asdasdasads

# All .o -> codesegment.o
$(CODESEG) : $(OBJS) | $(TOOLS)
	@$(PRINT)$(GREEN)Combining code objects into code segment$(ENDGREEN)$(ENDLINE)
	@$(LD) -o $@ -r $^ $(SEG_LDFLAGS)

# .o -> .elf
# $(ELF) : $(CODESEG) $(SEG_OBJS) $(LD_CPP) $(BOOT_OBJ) $(ENTRY_OBJ) $(ASSETS_OBJ) $(SOUND_OBJS)
$(ELF) : $(LD_CPP) $(BOOT_OBJ) $(ENTRY_OBJ) $(ASSETS_OBJ) $(SOUND_OBJS) $(OBJS)
	@$(PRINT)$(GREEN)Linking elf file:$(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(LD) $(LDFLAGS) -o $@ $(OBJS) $(SEG_LDFLAGS) $(OPT_FLAGS)
#	@$(LD) $(LDFLAGS) -o $@

# .elf -> .z64
$(Z64) : $(ELF)
	@$(PRINT)$(GREEN)Creating z64: $(ENDGREEN)$(BLUE)$@$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) $< $@ -O binary $(OCOPYFLAGS)
	@$(PRINT)$(GREEN)Calculating checksums$(ENDGREEN)$(ENDLINE)
	@$(CKSUM) $@
	@$(PRINT)$(WHITE)ROM Built!$(ENDWHITE)$(ENDLINE)

# Preprocess LD script
$(LD_CPP) : $(LD_SCRIPT) | $(BUILD_ROOT)
	@$(PRINT)$(GREEN)Preprocessing linker script$(ENDGREEN)$(ENDLINE)
	@$(CPP) $(LDCPPFLAGS) -MMD -MP -MT $@ -MF $@.d -o $@ $<

# Compile assetpack
$(ASSETPACK) :
	@$(PRINT)$(GREEN)Compiling assetpack$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/assetpack

# Pack assets
$(ASSETS_BIN) : $(ASSETS_OUT)
	@$(PRINT)$(GREEN)Packing assets$(ENDGREEN)$(ENDLINE)
	@$(ASSETPACK) $(BUILD_ROOT)/$(ASSET_ROOT) $@ $(ASSETS_TXT)

# Create gperf words file
$(ASSETS_GPERF) : $(ASSETS_BIN)
	@$(PRINT)$(GREEN)Creating assets gperf input file$(ENDGREEN)$(ENDLINE)
	@$(CAT) $(PLATFORM_DIR)/asset_gperf_header.txt $(ASSETS_TXT) > $@

# Run gperf
$(ASSETS_CPP) : $(ASSETS_GPERF)
	@$(PRINT)$(GREEN)Running gperf for asset table$(ENDGREEN)$(ENDLINE)
	@$(GPERF) -L C++ -Z FileRecords -N get_offset -C -c -m 1000 -t $< > $@

# Compile gltf64
$(GLTF64) :
	@$(PRINT)$(GREEN)Compiling glTF64$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/gltf64

# Convert models
$(MODELS_OUT) : $(BUILD_ROOT)/% : %.gltf | $(BUILD_DIRS) $(GLTF64)
	@$(PRINT)$(GREEN)Converting model: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(GLTF64) $< $@ $(BUILD_ROOT)/$(ASSET_ROOT)

# Compile soundconv
$(SOUNDCONV) :
	@$(PRINT)$(GREEN)Compiling soundconv$(ENDGREEN)$(ENDLINE)
	@$(MAKE) -C tools/soundconv

# Convert sounds
$(WAVETABLES) : $(SAMPLES) $(SFX_JSON) | $(BUILD_DIRS) $(SOUNDCONV)
	@$(PRINT)$(GREEN)Converting sound files$(ENDGREEN)$(ENDLINE)
	@$(SOUNDCONV) $(SAMPLES_DIR) $(SFX_JSON) $(BUILD_ROOT)

# Convert images
$(IMAGES_OUT) : $(BUILD_ROOT)/% : %.png | $(BUILD_DIRS)
	@n64graphics -i $@ -g $< -f rgba16

# Pretend to make the fxbank (it's generated by soundconv along with the wavebank)
$(FXBANK) : $(WAVETABLES)
	@touch $@

# Pretend to make the ptrbank (it's generated by soundconv along with the wavebank)
$(PTRBANK) : $(WAVETABLES)
	@touch $@
  
# fxbank -> .o
$(FXBANK_O) : $(FXBANK) | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying built binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) -B mips:$(MIPS_ARCH) $< $@
	@$(OBJCOPY) --rename-section .data=.bindata $@
	@mips-linux-gnu-strip $@ -N asdasdasads

# ptrbank -> .o
$(PTRBANK_O) : $(PTRBANK) | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying built binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) -B mips:$(MIPS_ARCH) $< $@
	@$(OBJCOPY) --rename-section .data=.bindata $@
	@mips-linux-gnu-strip $@ -N asdasdasads

# wavetables -> .o
$(WAVETABLES_O) : $(WAVETABLES) | $(BUILD_DIRS)
	@$(PRINT)$(GREEN)Objcopying built binary file: $(ENDGREEN)$(BLUE)$<$(ENDBLUE)$(ENDLINE)
	@$(OBJCOPY) -I binary -O $(FORMAT) -B mips:$(MIPS_ARCH) $< $@
	@$(OBJCOPY) --rename-section .data=.bindata $@
	@mips-linux-gnu-strip $@ -N asdasdasads

clean:
	@$(PRINT)$(YELLOW)Cleaning build$(ENDYELLOW)$(ENDLINE)
	@$(RMDIR) $(BUILD_ROOT) $(RMDIR_OPTS)
	
srcclean:
	@$(PRINT)$(YELLOW)Cleaning built sources$(ENDYELLOW)$(ENDLINE)
	@$(RMDIR) $(BUILD_ROOT)/$(SRC_ROOT) $(BUILD_ROOT)/$(PLATFORM_DIR) $(RMDIR_OPTS)
	@$(RM) -f $(Z64) $(ELF) $(CODESEG)

load: $(Z64)
	@$(PRINT)$(GREEN)Loading $(Z64) onto flashcart$(ENDGREEN)$(ENDLINE)
	@$(RUN) $(UNFLOADER) $(Z64) -d

.PHONY: all clean load

-include $(D_FILES)

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
