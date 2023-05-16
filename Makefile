# Directories
SRC_DIR     := src
BUILD_DIR   := build
INCLUDE_DIR := include
BIN_DIR     := bin
EXEC_DIR    := $(SRC_DIR)/exec
MAN_DIR     := src/man
ROFF_DIR    := man

# Compiler Options
CC       ?= clang
CFLAGS   += -g -std=gnu99 -Wpedantic -Wunused-result -Wno-gnu-zero-variadic-macro-arguments
LDFLAGS  += -fPIE

# Executable-specific flags
finit_FLAGS := -static

# File lists
SOURCE_FILES  := $(wildcard $(SRC_DIR)/*.c)
EXEC_FILES    := $(wildcard $(EXEC_DIR)/*)
OBJ_FILES     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCE_FILES))
BIN_FILES     := $(patsubst $(EXEC_DIR)/%.c,$(BIN_DIR)/%,$(EXEC_FILES)) \
				 $(patsubst $(EXEC_DIR)/%.sh,$(BIN_DIR)/%,$(EXEC_FILES))
INCLUDE_FILES := $(wildcard $(INCLUDE_DIR)/*.h)

MAN_FILES     := $(wildcard $(MAN_DIR)/*)
ROFF_FILES    := $(patsubst $(MAN_DIR)/%.md,$(ROFF_DIR)/%,$(MAN_FILES)) \
				 $(patsubst $(MAN_DIR)/%.roff,$(ROFF_DIR)/%,$(MAN_FILES))

# Intermediate directories
INTERMED_DIRS := $(BIN_DIR) $(BUILD_DIR) $(ROFF_DIR)

# Magic targets
.PHONY: all clean manual binary

.PRECIOUS: $(OBJ_FILES)


# Default target
all: compile_flags.txt binary manual

# Clean target
clean:
	rm -rf $(INTERMED_DIRS)

binary: $(BIN_FILES)

manual: $(ROFF_FILES)

# Directory rules
$(INTERMED_DIRS):
	mkdir -p $@

# Object rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_FILES) | $(BUILD_DIR)
	$(CC) -o $@ -c -I$(INCLUDE_DIR) $(CFLAGS) $<

# Executables
$(BIN_DIR)/%: $(EXEC_DIR)/%.c $(INCLUDE_FILES) $(OBJ_FILES) | $(BIN_DIR)
	$(CC) -o $@ -I$(INCLUDE_DIR) $(CFLAGS) $< $(OBJ_FILES) $($(notdir $@)_FLAGS) $(LDFLAGS)

$(BIN_DIR)/%: $(EXEC_DIR)/%.sh | $(BIN_DIR)
	cp $< $@
	chmod +x $@

# Manual targets

$(ROFF_DIR)/%: $(MAN_DIR)/%.md | $(ROFF_DIR)
	md2man-roff $< > $@

$(ROFF_DIR)/%: $(MAN_DIR)/%.roff | $(ROFF_DIR)
	cp $< $@

# Debug
compile_flags.txt: 
	echo $(CFLAGS) | tr " " "\n" > compile_flags.txt