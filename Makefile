VERSION = 0.3.2

# Directories
SRC_DIR     := src
BUILD_DIR   := build
INCLUDE_DIR := include
BIN_DIR     := bin
EXEC_DIR    := src/exec
MAN_DIR     := src/man
ROFF_DIR    := man
TEMPL_DIR   := src/docs
DOCS_DIR    := docs
ASSETS_DIR  := assets
DOC_AST_DIR := docs/assets

# Compiler Options
CC       ?= gcc
CFLAGS   += -I$(INCLUDE_DIR) -DVERSION=\"$(VERSION)\" -g -std=gnu99 -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments
LDFLAGS  += -fPIE

SED      ?= sed
PYTHON   ?= python3

# Executable-specific flags
finit_FLAGS := -static

# File lists
SOURCE_FILES  := $(wildcard $(SRC_DIR)/*.c)
EXEC_FILES    := $(wildcard $(EXEC_DIR)/*)
OBJ_FILES     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCE_FILES))
BIN_FILES     := $(patsubst $(EXEC_DIR)/%.c,$(BIN_DIR)/%,$(EXEC_FILES)) \
				 $(patsubst $(EXEC_DIR)/%.sh,$(BIN_DIR)/%,$(EXEC_FILES)) \
				 $(patsubst $(EXEC_DIR)/%.lnk,$(BIN_DIR)/%,$(EXEC_FILES))
INCLUDE_FILES := $(wildcard $(INCLUDE_DIR)/*.h)

MAN_FILES     := $(wildcard $(MAN_DIR)/*)
ROFF_FILES    := $(patsubst $(MAN_DIR)/%.md,$(ROFF_DIR)/%,$(MAN_FILES)) \
				 $(patsubst $(MAN_DIR)/%.roff,$(ROFF_DIR)/%,$(MAN_FILES))
				 
TEMPL_FILES   := $(wildcard $(TEMPL_DIR)/*.html)
DOCS_FILES    := $(patsubst $(TEMPL_DIR)/%.html,$(DOCS_DIR)/%.html,$(TEMPL_FILES))

# Intermediate directories
INTERMED_DIRS := $(BIN_DIR) $(BUILD_DIR) $(ROFF_DIR) $(DOCS_DIR)

# Magic targets
.PHONY: all clean manual binary documentation
 
.PRECIOUS: $(OBJ_FILES)


# Default target
all: compile_flags.txt binary manual documentation

# Clean target
clean:
	rm -rf $(INTERMED_DIRS)

binary: $(BIN_FILES)

manual: $(ROFF_FILES)

documentation: $(DOCS_FILES)

# Directory rules
$(INTERMED_DIRS):
	mkdir -p $@
	
$(DOC_AST_DIR): $(ASSETS_DIR) | $(DOCS_DIR)
	cp -rv $< $@


# Object rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_FILES) | $(BUILD_DIR)
	$(CC) -o $@ -c $(CFLAGS) $<

# Executables
$(BIN_DIR)/%: $(EXEC_DIR)/%.c $(INCLUDE_FILES) $(OBJ_FILES) | $(BIN_DIR)
	$(CC) -o $@ $(CFLAGS) $< $(OBJ_FILES) $($(notdir $@)_FLAGS) $(LDFLAGS)

$(BIN_DIR)/%: $(EXEC_DIR)/%.sh | $(BIN_DIR)
	cp $< $@
	chmod +x $@

$(BIN_DIR)/%: $(EXEC_DIR)/%.lnk | $(BIN_DIR)
	ln -sf $(shell cat $<) $@	
	
$(DOCS_DIR)/%.html: $(TEMPL_DIR)/%.html $(DOC_AST_DIR) | $(DOCS_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) make-docs.py > $@


# Manual targets

$(ROFF_DIR)/%: $(MAN_DIR)/%.md | $(ROFF_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< | md2man-roff > $@

$(ROFF_DIR)/%: $(MAN_DIR)/%.roff | $(ROFF_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< > $@

# Debug
compile_flags.txt: 
	echo $(CFLAGS) | tr " " "\n" > compile_flags.txt