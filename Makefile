PROG=	wtrd
CFLAGS=	-Wall -Werror
LDADD=	-lprocstat

MK_MAN=	no

.include <bsd.prog.mk>
