PROG=	wtrd
SRCS=	config.c wtrd.c
CFLAGS=	-Wall -Werror
LDADD=	-lprocstat

GLIB_FLAGS!=	pkg-config --cflags glib-2.0
GLIB_LDADD!=	pkg-config --libs glib-2.0

CFLAGS+=	${GLIB_FLAGS}
LDADD+=		${GLIB_LDADD}

MK_MAN=	no

.include <bsd.prog.mk>
