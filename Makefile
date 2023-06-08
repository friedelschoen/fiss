include mk/config.mk
include mk/binary.mk

VERSION := 0.3.3

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

binary: $(BIN_FILES)

manual: $(ROFF_FILES)

documentation: $(DOCS_FILES)

include mk/target.mk
include mk/install.mk
