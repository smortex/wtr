PROG=		wtrd
MAN=

SRCS=		wtrd.c
CFLAGS=		-I../libwtr
LDADD=		../libwtr/libwtr.a

.if ${OS} == "FreeBSD"
SRCS+=		freebsd.c
LDADD+=		-lprocstat ../libwtr/libwtr.a
.elif ${OS} == "Linux"
SRCS+=		linux.c
LIBBSD_CFLAGS!=	pkg-config --cflags libbsd-overlay
LIBBSD_LDADD!=	pkg-config --libs libbsd-overlay
CFLAGS+=	${LIBBSD_CFLAGS}
LDADD+=		${LIBBSD_LDADD}
.else
.error "Unsupported OS ${OS}"
.endif

GLIB_CFLAGS!=	pkg-config --cflags glib-2.0
GLIB_LDADD!=	pkg-config --libs glib-2.0
CFLAGS+=	${GLIB_CFLAGS}
LDADD+=		${GLIB_LDADD}

SQLITE3_LDADD!=	pkg-config --libs sqlite3
LDADD+=		${SQLITE3_LDADD}

.include <bsd.prog.mk>
