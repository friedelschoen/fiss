include config.mk
include mk/binary.mk

VERSION := 0.3.3

SRC_DIR           := src
INCLUDE_DIR       := include
BIN_DIR           := bin
MAN_DIR           := man
DOCS_DIR          := docs
ASSETS_DIR        := assets
TOOLS_DIR         := tools
TARGET_DIR        := target

TARGET_OBJECT_DIR := $(TARGET_DIR)/obj
TARGET_BIN_DIR    := $(TARGET_DIR)/bin
TARGET_MAN_DIR    := $(TARGET_DIR)/man
TARGET_DOCS_DIR   := $(TARGET_DIR)/docs
TARGET_ASSETS_DIR := $(TARGET_DOCS_DIR)/assets

TARGET_DIRS   := $(TARGET_DIR) $(TARGET_OBJECT_DIR) $(TARGET_BIN_DIR) \
                 $(TARGET_DOCS_DIR) $(TARGET_MAN_DIR)

SOURCE_FILES  := $(wildcard $(SRC_DIR)/*.c)
EXEC_FILES    := $(wildcard $(BIN_DIR)/*)
OBJ_FILES     := $(patsubst $(SRC_DIR)/%.c,$(TARGET_OBJECT_DIR)/%.o,$(SOURCE_FILES))
BIN_FILES     := $(patsubst %,$(TARGET_BIN_DIR)/%,$(BINARIES))
INCLUDE_FILES := $(wildcard $(INCLUDE_DIR)/*.h)

MAN_FILES     := $(wildcard $(MAN_DIR)/*.txt)
TEMPL_FILES   := $(wildcard $(DOCS_DIR)/*.txt)

ROFF_FILES    := $(patsubst $(MAN_DIR)/%.txt,$(TARGET_MAN_DIR)/%,$(MAN_FILES))
DOCS_FILES    := $(patsubst $(DOCS_DIR)/%.txt,$(TARGET_DOCS_DIR)/%.html,$(TEMPL_FILES)) \
                 $(patsubst $(MAN_DIR)/%.txt,$(TARGET_DOCS_DIR)/%.html,$(MAN_FILES))

CFLAGS         += -I$(INCLUDE_DIR) -DVERSION=\"$(VERSION)\" -g -std=gnu99
LDFLAGS        +=

ifeq ($(VERBOSE),)
SILENT := @
endif

# Magic targets
.PHONY: all clean manual binary documentation
 
.PRECIOUS: $(OBJ_FILES) $(patsubst $(BIN_DIR)/%.c,$(TARGET_OBJECT_DIR)/%.o,$(EXEC_FILES))


# Default target
all: compile_flags.txt binary manual documentation

# Clean target
clean:
	@echo "[RM] $(TARGET_DIRS)"
	$(SILENT)rm -rf $(TARGET_DIRS)
	$(SILENT)rm config.mk

binary: $(BIN_FILES)

manual: $(ROFF_FILES)

documentation: $(DOCS_FILES)

include mk/target.mk
include mk/install.mk
