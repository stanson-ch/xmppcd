SUBDIRS = src

docdir = $(datarootdir)/doc/@PACKAGE@
doc_DATA = README COPYING AUTHORS

confdir = $(sysconfdir)
conf_DATA = xmppcd.conf.example

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/spool/xmppcd/in
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/spool/xmppcd/out
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/spool/xmppcd/sent
