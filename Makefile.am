SUBDIRS =								\
	icons								\
	panel-plugin 							\
	po

distclean-local:
	rm -rf *.cache *~

distuninstallcheck_listfiles = 						\
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST =								\
	README.md							\								
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
