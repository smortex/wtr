SUBDIR=	libwtr \
	wtr \
	wtrd

style:
	astyle --options=.astylerc "./*.h" "./*.c"

.include <bsd.subdir.mk>
