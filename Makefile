# Directories
SRC_DIR     := src
BUILD_DIR   := build
INCLUDE_DIR := include
BIN_DIR     := bin
EXEC_DIR    := $(SRC_DIR)/exec
SCRIPT_DIR  := $(SRC_DIR)/script
MAN_DIR     := man
ROFF_DIR    := usr/share/man

# Compiler Options
CC      := gcc
CCFLAGS := -I$(INCLUDE_DIR) -Wall -Wextra
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

MAN_FILES     := $(wildcard $(MAN_DIR)/*)
ROFF_FILES    := $(patsubst $(MAN_DIR)/%.md,$(ROFF_DIR)/%,$(MAN_FILES)) \
				 $(patsubst $(MAN_DIR)/%.roff,$(ROFF_DIR)/%,$(MAN_FILES))

# Intermediate directories
INTERMED_DIRS := $(BIN_DIR) $(BUILD_DIR) $(ROFF_DIR)

# Magic targets
.PHONY: all clean manual

.PRECIOUS: $(OBJ_FILES)


# Default target
all: compile_flags.txt $(BIN_FILES) manual

# Clean target
clean:
	rm -rf $(INTERMED_DIRS)

manual: $(ROFF_FILES)

# Directory rules
$(INTERMED_DIRS):
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

# Manual targets

$(ROFF_DIR)/%: $(MAN_DIR)/%.md | $(ROFF_DIR)
	md2man-roff $< > $@

$(ROFF_DIR)/%: $(MAN_DIR)/%.roff | $(ROFF_DIR)
	cp $< $@

# Debug
compile_flags.txt: 
	echo $(CCFLAGS) | tr " " "\n" > compile_flags.txt