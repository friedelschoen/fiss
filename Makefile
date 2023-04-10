# Directories
SRC_DIR     := src
BUILD_DIR   := build
INCLUDE_DIR := include
BIN_DIR     := bin
EXEC_DIR    := $(SRC_DIR)/exec
SCRIPT_DIR  := $(SRC_DIR)/script

# Compiler Options
CC      := gcc
CCFLAGS := -I$(INCLUDE_DIR) -Wall -Wextra -g
LFLAGS  :=

# Executable-specific flags
finit_FLAGS := -static

# File lists
SOURCE_FILES  := $(wildcard $(SRC_DIR)/*.c)
EXEC_FILES    := $(wildcard $(EXEC_DIR)/*.c)
OBJ_FILES     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCE_FILES))
SCRIPT_FILES  := $(wildcard $(SCRIPT_DIR)/*)
BIN_FILES     := $(patsubst $(EXEC_DIR)/%.c,$(BIN_DIR)/%,$(EXEC_FILES)) \
			     $(patsubst $(SCRIPT_DIR)/%.sh,$(BIN_DIR)/%,$(SCRIPT_FILES)) \
			     $(patsubst $(SCRIPT_DIR)/%.lnk,$(BIN_DIR)/%,$(SCRIPT_FILES))
INCLUDE_FILES := $(wildcard $(INCLUDE_DIR)/*.h)

# Magic targets
.PHONY: all clean

.PRECIOUS: $(OBJ_FILES)


# Default target
all: compile_flags.txt $(BIN_FILES)

# Clean target
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

# Directory rules
$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

# Object rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_FILES) | $(BUILD_DIR)
	$(CC) -o $@ -c $(CCFLAGS) $<

# Executables
$(BIN_DIR)/%: $(EXEC_DIR)/%.c $(INCLUDE_FILES) $(OBJ_FILES) | $(BIN_DIR)
	$(CC) -o $@ $(CCFLAGS) $< $(OBJ_FILES) $($(notdir $@)_FLAGS) $(LFLAGS)

$(BIN_DIR)/%: $(SCRIPT_DIR)/%.lnk | $(BIN_DIR)
	ln -s $(shell cat $<) $@

$(BIN_DIR)/%: $(SCRIPT_DIR)/%.sh | $(BIN_DIR)
	cp $< $@
	chmod +x $@

# Debug
compile_flags.txt: 
	echo $(CCFLAGS) | tr " " "\n" > compile_flags.txt

# debug

.PHONY: emulator

emulator: $(BIN_FILES)
	cp -v bin/* rootfs/sbin/
	cd rootfs && find | cpio -oH newc -R root:root | zstd > ../build/rootfs.cpio

	qemu-system-x86_64 -accel kvm -kernel ~/linux-void/vmlinuz-6.1.21_1 -initrd build/rootfs.cpio -m 4096 -append 'console=ttyS0 edd=off quiet' -serial stdio
#	qemu-system-x86_64 -accel kvm -kernel /boot/vmlinuz-6.1.21_1 -initrd build/rootfs.cpio -m 4096 -append 'console=ttyS0 edd=off quiet' -serial stdio