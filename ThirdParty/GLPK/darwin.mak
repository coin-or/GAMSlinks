## darwin.mak (Darwin single-threaded static library, IBM xlc) ##

CFLAGS = -O -DNDEBUG

.c.o:
	xlc_r $(CFLAGS) -Iinclude -o$*.o -c $*.c

all: libglpk.a

libglpk.a: src/glpavl.o \
        src/glpbfi.o \
        src/glpbfx.o \
        src/glpdmp.o \
        src/glpgmp.o \
        src/glphbm.o \
        src/glpiet.o \
        src/glpinv01.o \
        src/glpinv02.o \
        src/glpios01.o \
        src/glpios02.o \
        src/glpios03.o \
        src/glpipm.o \
        src/glpipp01.o \
        src/glpipp02.o \
        src/glplib01.o \
        src/glplib02.o \
        src/glplib03.o \
        src/glplib04.o \
        src/glplib05.o \
        src/glplib06.o \
        src/glplib07.o \
        src/glplpp01.o \
        src/glplpp02.o \
        src/glplpx01.o \
        src/glplpx02.o \
        src/glplpx03.o \
        src/glplpx04.o \
        src/glplpx05.o \
        src/glplpx06.o \
        src/glplpx07.o \
        src/glplpx08.o \
        src/glplpx09.o \
        src/glplpx10.o \
        src/glplpx11.o \
        src/glplpx12.o \
        src/glplpx13.o \
        src/glplpx14.o \
        src/glplpx15.o \
        src/glplpx16.o \
        src/glplpx17.o \
        src/glplpx18.o \
        src/glpluf01.o \
        src/glpluf02.o \
        src/glplux.o \
        src/glpmat.o \
        src/glpmip01.o \
        src/glpmip02.o \
        src/glpmpl01.o \
        src/glpmpl02.o \
        src/glpmpl03.o \
        src/glpmpl04.o \
        src/glpqmd.o \
        src/glprng.o \
        src/glpspx01.o \
        src/glpspx02.o \
        src/glpssx01.o \
        src/glpssx02.o \
        src/glpstr.o \
        src/glptsp.o
	ar -r libglpk.a src/*.o
