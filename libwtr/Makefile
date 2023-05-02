LIB=		wtr
MAN=

SRCS=		config.c database.c utils.c
CFLAGS+=	-fPIC

GLIB_CFLAGS!=	pkg-config --cflags glib-2.0
CFLAGS+=	${GLIB_CFLAGS}

SQLITE3_CFLAGS!=pkg-config --cflags sqlite3
CFLAGS+=	${SQLITE3_CFLAGS}

.include <bsd.lib.mk>
