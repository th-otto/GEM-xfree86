m68k-atari-mint-gcc  -O2 -fomit-frame-pointer   \
	-I../../include -I../../exports/include/X11  \
	-I../.. -I../../exports/include  -Dtypeof=__typeof__ -D_GNU_SOURCE   -DFUNCPROTO=15 -DNARROWPROTO     -DCPP_PROGRAM="\"m68k-atari-mint-cpp\"" \
	-o imake imake.c
