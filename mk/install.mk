.PHONY: install install-binary install-documentation install-binary install-share install-etc

install: install-binary \
		 install-documentation \
		 install-manual \
		 install-share \
		 install-etc
		 
install-binary: $(BIN_FILES)
ifneq ($(INSTALL_SBIN),)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_SBIN)
	@for file in $(BIN_FILES); do \
	    install -v -m 755 $$file $(INSTALL_PREFIX)/$(INSTALL_SBIN); \
	done
endif

install-documentation: $(DOCS_FILES)
ifneq ($(INSTALL_DOCS),)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_DOCS)
	@for file in $(DOCS_FILES); do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_DOCS); \
	done
endif

install-manual: $(ROFF_FILES)
ifneq ($(INSTALL_MAN8),)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_MAN8)
	@for file in $(ROFF_FILES); do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_MAN8); \
	done
endif

install-share:
ifneq ($(INSTALL_SHARE),)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_SHARE)/fiss
	@for file in share/fiss/*; do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_SHARE)/fiss; \
	done
endif

install-etc:
ifneq ($(INSTALL_ETC),)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/service.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/start.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/stop.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/zzz.d/resume
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/zzz.d/suspend
endif
