SRC_DIR           := src
INCLUDE_DIR       := include
BIN_DIR           := bin
MAN_DIR           := man
DOCS_DIR          := docs
ASSETS_DIR        := assets
TOOLS_DIR         := tools
TARGET_DIR        := target

INSTALL_PREFIX    := /
INSTALL_SBIN      := /sbin
INSTALL_SHARE     := /usr/share
INSTALL_MAN8      := /usr/share/man/man8
INSTALL_DOCS      := /usr/share/doc/fiss
INSTALL_ETC       := /etc

TARGET_OBJECT_DIR := $(TARGET_DIR)/obj
TARGET_BIN_DIR    := $(TARGET_DIR)/bin
TARGET_MAN_DIR    := $(TARGET_DIR)/man
TARGET_DOCS_DIR   := $(TARGET_DIR)/docs
TARGET_ASSETS_DIR := $(TARGET_DOCS_DIR)/assets

TARGET_DIRS   := $(TARGET_DIR) $(TARGET_OBJECT_DIR) $(TARGET_BIN_DIR) \
                 $(TARGET_DOCS_DIR) $(TARGET_MAN_DIR)

# Compiler Options
CC       ?= gcc
CFLAGS   += -I$(INCLUDE_DIR) -DVERSION=\"$(VERSION)\" -g -std=gnu99 -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments
LDFLAGS  += -fPIE

# Utilities
SED       ?= sed
PYTHON    ?= python3
AWK       ?= awk
MAKE_DOCS ?= $(TOOLS_DIR)/make-docs.py
MAKE_MAN  ?= $(TOOLS_DIR)/make-man.py

VERBOSE :=