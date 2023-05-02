PROG=	wtrd
SRCS=	config.c database.c wtrd.c
CFLAGS=	-Wall -Werror
LDADD=	-lprocstat

GLIB_CFLAGS!=	pkg-config --cflags glib-2.0
GLIB_LDADD!=	pkg-config --libs glib-2.0

CFLAGS+=	${GLIB_CFLAGS}
LDADD+=		${GLIB_LDADD}

SQLITE3_CFLAGS!=pkg-config --cflags sqlite3
SQLITE3_LDADD!=	pkg-config --libs sqlite3

CFLAGS+=	${SQLITE3_CFLAGS}
LDADD+=		${SQLITE3_LDADD}

MK_MAN=	no

.include <bsd.prog.mk>
