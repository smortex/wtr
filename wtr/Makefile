PROG=		wtr
MAN=

SRCS=		wtr.c
CFLAGS=		-I../libwtr
LDADD=		../libwtr/libwtr.a

GLIB_CFLAGS!=	pkg-config --cflags glib-2.0
GLIB_LDADD!=	pkg-config --libs glib-2.0
CFLAGS+=	${GLIB_CFLAGS}
LDADD+=		${GLIB_LDADD}

SQLITE3_LDADD!=	pkg-config --libs sqlite3
LDADD+=		${SQLITE3_LDADD}

.include <bsd.prog.mk>
