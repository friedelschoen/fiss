.PHONY: install install-binary install-documentation install-binary install-share install-etc

install: install-binary \
		 install-documentation \
		 install-manual \
		 install-share \
		 install-etc
		 
install-binary: $(BIN_FILES)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_SBIN)
	@for file in $(BIN_FILES); do \
	    install -v -m 755 $$file $(INSTALL_PREFIX)/$(INSTALL_SBIN); \
	done

install-documentation: $(DOCS_FILES)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_DOCS)
	@for file in $(DOCS_FILES); do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_DOCS); \
	done

install-manual: $(ROFF_FILES)
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_MAN8)
	@for file in $(ROFF_FILES); do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_MAN8); \
	done

install-share:
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_SHARE)/fiss
	@for file in share/fiss/*; do \
	    install -v -m 644 $$file $(INSTALL_PREFIX)/$(INSTALL_SHARE)/fiss; \
	done

install-etc:
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/service.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/start.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/stop.d
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/zzz.d/resume
	@install -vd $(INSTALL_PREFIX)/$(INSTALL_ETC)/zzz.d/suspend
