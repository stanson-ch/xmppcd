#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(xmppcd, 0.1, me@stanson.ch)

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)
AC_SUBST([sysconfdir], ['/etc'])
AC_SUBST([localstatedir], ['/var'])

# Checks for programs.
AC_PROG_CC
AC_PROG_MKDIR_P

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h stdint.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset socket strdup strerror strrchr strstr strtol strtoul])

PKG_CHECK_MODULES(LIBSTROPHE, libstrophe, dummy=yes, AC_MSG_ERROR(libstrophe is required))
AC_SUBST(LIBSTROPHE_CFLAGS)
AC_SUBST(LIBSTROPHE_LIBS)

AC_DEFINE_UNQUOTED(XMPPCD_CONFIG, ["${sysconfdir}/xmppcd.conf"], [Define xmppcd.conf file path])
AC_DEFINE_UNQUOTED(XMPPCD_SPOOL, ["${localstatedir}/spool/xmppcd"], [Define xmppcd spool directory path])

AC_OUTPUT(Makefile src/Makefile xmppcd.conf.example src/xmpp-send)
