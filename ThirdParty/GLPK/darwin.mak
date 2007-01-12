## darwin.mak (Darwin single-threaded static library, IBM xlc) ##

CFLAGS = -O -DNDEBUG

.c.o:
	xlc_r $(CFLAGS) -Iinclude -o$*.o -c $*.c

all: libglpk.a

libglpk.a: src/glpavl.o \
        src/glpdmp.o \
        src/glphbm.o \
        src/glpiet.o \
        src/glpinv.o \
        src/glpios1.o \
        src/glpios2.o \
        src/glpios3.o \
        src/glpipm.o \
        src/glpipp1.o \
        src/glpipp2.o \
        src/glplib1a.o \
        sysdep/gnu/glplib1b.o \
        src/glplib2.o \
        src/glplib3.o \
        src/glplib4.o \
        src/glplpp1.o \
        src/glplpp2.o \
        src/glplpx1.o \
        src/glplpx2.o \
        src/glplpx3.o \
        src/glplpx4.o \
        src/glplpx5.o \
        src/glplpx6a.o \
        src/glplpx6b.o \
        src/glplpx6c.o \
        src/glplpx6d.o \
        src/glplpx7.o \
        src/glplpx7a.o \
        src/glplpx8a.o \
        src/glplpx8b.o \
        src/glplpx8c.o \
        src/glplpx8d.o \
        src/glplpx8e.o \
        src/glpluf.o \
        src/glpmat.o \
        src/glpmip1.o \
        src/glpmip2.o \
        src/glpmpl1.o \
        src/glpmpl2.o \
        src/glpmpl3.o \
        src/glpmpl4.o \
        src/glpqmd.o \
        src/glprng.o \
        src/glpspx1.o \
        src/glpspx2.o \
        src/glpstr.o \
        src/glptsp.o
	ar -r libglpk.a src/*.o sysdep/gnu/*.o
