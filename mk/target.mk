# Directory rules
$(TARGET_DIRS):
	@echo "[MKDIR] $@"
	$(SILENT)mkdir -p $@

$(TARGET_ASSETS_DIR): $(ASSETS_DIR) | $(TARGET_DOCS_DIR)
	@echo "[CP] $@"
	$(SILENT)mkdir -p $@
	$(SILENT)cp -rv $</* $@


# Object rules
$(TARGET_OBJECT_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_FILES) | $(TARGET_OBJECT_DIR)
	@echo "[CC] $@"
	$(SILENT)$(CC) -o $@ -c $(CFLAGS) $<

$(TARGET_OBJECT_DIR)/%.o: $(BIN_DIR)/%.c $(INCLUDE_FILES) | $(TARGET_OBJECT_DIR)
	@echo "[CC] $@"
	$(SILENT)$(CC) -o $@ -c $(CFLAGS) $<

# Executables
$(TARGET_BIN_DIR)/%: $(TARGET_OBJECT_DIR)/%.o $(OBJ_FILES) | $(TARGET_BIN_DIR)
	@echo "[LD] $@"
	$(SILENT)$(CC) -o $@ $< $(patsubst %,$(TARGET_OBJECT_DIR)/%,$($(notdir $@)_OBJECTS)) $($(notdir $@)_FLAGS) $(LDFLAGS)

$(TARGET_BIN_DIR)/%: $(BIN_DIR)/%.sh | $(TARGET_BIN_DIR)
	@echo "[CP] $@"
	$(SILENT)cp $< $@
	$(SILENT)chmod +x $@

$(TARGET_BIN_DIR)/%: $(BIN_DIR)/%.lnk | $(TARGET_BIN_DIR)
	@echo "[LN] $@"
	$(SILENT)ln -sf $(shell cat $<) $@

# Documentation and Manual
$(TARGET_DOCS_DIR)/%.html: $(DOCS_DIR)/%.txt $(TARGET_ASSETS_DIR) $(MAKE_DOCS) | $(TARGET_DOCS_DIR)
	@echo "[MKDOC] $@"
	$(SILENT)$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_DOCS) > $@

$(TARGET_DOCS_DIR)/%.html: $(MAN_DIR)/%.txt $(TARGET_ASSETS_DIR) $(MAKE_DOCS) | $(TARGET_DOCS_DIR)
	@echo "[MKDOC] $@"
	$(SILENT)$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_DOCS) > $@

$(TARGET_MAN_DIR)/%: $(MAN_DIR)/%.txt $(MAKE_MAN) | $(TARGET_MAN_DIR)
	@echo "[MKMAN] $@"
	$(SILENT)$(SED) 's/%VERSION%/$(VERSION)/' $< | $(PYTHON) $(MAKE_MAN) | $(AWK) '/./ { print }' > $@

# Debug
compile_flags.txt: Makefile
	@echo "[ECHO] $@"
	$(SILENT)echo $(CFLAGS) | tr " " "\n" > compile_flags.txt