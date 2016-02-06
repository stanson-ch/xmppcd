# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall -O2 @LIBSTROPHE_CFLAGS@
AM_LDFLAGS = @LIBSTROPHE_LIBS@

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile

bin_PROGRAMS = xmppcd
bin_SCRIPTS = xmpp-send
xmppcd_SOURCES = 	main.c \
			conf.c \
			log.c

xmppcd_LDADD = @LIBSTROPHE_LIBS@

CLEANFILES = $(bin_SCRIPTS)
