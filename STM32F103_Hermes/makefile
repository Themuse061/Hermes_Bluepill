# ----- Basic Info ----- #
DEVICE := stm32f103c6t6
project_binary_name := Hermes_Bluepill
flash_size	:= 128000
ram_size 	:= 16000

# ----- Compiler and Linker Flags ----- #
ARCH_FLAGS += -O0
ARCH_FLAGS += -g
CPPFLAGS += -g
CPPFLAGS += -O0

override LDSCRIPT = $(assorted_folder)/linker.ld
LDFLAGS += -nostdlib
LDFLAGS += -lc -lgcc -lnosys
CPPFLAGS += -DSTM32F1

add_module = libopencm3

# Include main makefile
cerys_directory := C:/embedded_programming
include $(cerys_directory)/_Cerys_Toolchain/cerys_makefile.mk
