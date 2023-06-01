VERSION := 0.3.3

# Directories
SRC_DIR     := src
BUILD_DIR   := build
INCLUDE_DIR := include
BIN_DIR     := bin
EXEC_DIR    := src/bin
MAN_DIR     := src/man
TEMPL_DIR   := src/docs
ROFF_DIR    := man
DOCS_DIR    := docs
ASSETS_DIR  := assets
DOC_AST_DIR := docs/assets
MAKE_DOCS   := make-docs.py
MAKE_MAN    := make-man.py

# Compiler Options
CC       ?= gcc
CFLAGS   += -I$(INCLUDE_DIR) -DVERSION=\"$(VERSION)\" -g -std=gnu99 -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments
LDFLAGS  += -fPIE

SED      ?= sed
PYTHON   ?= python3
AWK      ?= awk

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

MAN_FILES     := $(wildcard $(MAN_DIR)/*.txt)
TEMPL_FILES   := $(wildcard $(TEMPL_DIR)/*.txt)

ROFF_FILES    := $(patsubst $(MAN_DIR)/%.txt,$(ROFF_DIR)/%,$(MAN_FILES))
DOCS_FILES    := $(patsubst $(TEMPL_DIR)/%.txt,$(DOCS_DIR)/%.html,$(TEMPL_FILES)) \
				 $(patsubst $(MAN_DIR)/%.txt,$(DOCS_DIR)/%.html,$(MAN_FILES))

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
	cp -rv $</* $@


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

# Documentation and Manual
$(DOCS_DIR)/%.html: $(TEMPL_DIR)/%.txt $(DOC_AST_DIR) $(MAKE_DOCS) | $(DOCS_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_DOCS) > $@

$(DOCS_DIR)/%.html: $(MAN_DIR)/%.txt $(DOC_AST_DIR) $(MAKE_DOCS) | $(DOCS_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_DOCS) > $@

$(ROFF_DIR)/%: $(MAN_DIR)/%.txt $(MAKE_MAN) | $(ROFF_DIR)
	$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_MAN) | $(AWK) '/./ { print }' > $@

# Debug
compile_flags.txt: 
	echo $(CFLAGS) | tr " " "\n" > compile_flags.txt