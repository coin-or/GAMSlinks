/*
 * gcmach.h
 *
 * defines used to make the C I/O Library, and other C code
 * machine and compiler-independent
 *
 * Requires that a machine/compiler be defined,
 * either in the Makefile, or a previous include,
 * or as a symbol predefined by the compiler.
 *
 * One of the following are defined based on the machine/compiler being used.
 *    MSDOS      : if defined it is a system with MSDOS OS / file system
 *    UNIX       : if defined it is a system with UNIX OS / file system
 *
 * these are optional, and default to undefined
 *    NOSTRDUP   : standard C lib has no 'strdup'
 *    NOSTRICMP  : standard C lib has no 'stricmp'
 *    NOSTRNICMP : standard C lib has no 'strnicmp'
 *
 * The following _may_ be defined, if the defaults (given at end of file)
 * are wrong.  If no definition is made the defaults are used.
 *    S_CHAR     : signed character, 1 byte
 *    U_CHAR     : unsigned character, 1 byte
 *    INT_2      : short int (2 byte, signed)
 *    INT_4      : int       (4 byte, signed)
 *    DREAL      : 8 byte floating point quantity
 */

#if ! defined(_GC_MACH_H_)
#define _GC_MACH_H_

#if   defined(AIX)
# define UNIX

#elif defined(AXN)
# define MSDOS

#elif defined(AXU) || defined(AXU_CC) || defined(AXU_GCC)
# define UNIX

#elif defined(DAR) || defined(DII)
# define UNIX

#elif defined(HP7)
# define UNIX

#elif defined(LNX) || defined(LX3) || defined(LXI) || defined(LXY) || defined(SIG)
# define NOSTRICMP
# define NOSTRNICMP
# define UNIX

#elif defined(LEG) || defined(LEI)
# define NOSTRICMP
# define NOSTRNICMP
# define UNIX

#elif defined(LI4)			/* linux on iSeries */
# define NOSTRICMP
# define NOSTRNICMP
# define UNIX

#elif defined(LP4)			/* linux on pSeries */
# define NOSTRICMP
# define NOSTRNICMP
# define UNIX

#elif defined(LZ4)			/* linux on zSeries */
# define NOSTRICMP
# define NOSTRNICMP
# define UNIX

#elif defined(SG3) || defined(SG4) || defined(SG6) || defined(SG8) || defined(SGI)
# if ! defined(SGI)
#   define SGI
# endif
# define UNIX
# define NOSTRICMP
/* end SG3, SG4, SG6, SG8 */

#elif defined(S86)
# define UNIX

#elif defined(SOL) || defined(SOX)
# define UNIX

#elif defined(ITL) || defined(L95) || defined(WAT) || defined(WPL) || defined(VIS) || defined(WEI) || defined(MGW)
# define MSDOS

#else
# error "You must define exactly one machine/compiler for this build"
  !! should die with an error right here;
#endif /* defined() machine/compiler types */

/*
 * Now, these default declarations are used if nothing was put in above.
 * If you are doing a port and any of the #defines below fails,
 * you have to figure out if char is signed or unsigned, etc, and
 * #define it in the OS/compiler-specific #defines above
 */

#ifndef S_CHAR                  /* if not overridden above */
#define S_CHAR signed char
#endif
#ifndef U_CHAR                  /* ditto */
#define U_CHAR unsigned char
#endif
#ifndef INT_2                   /* ditto */
#define INT_2 short int         /* 2-byte signed int */
#endif
#ifndef INT_4                   /* ditto */
#define INT_4 int               /* 4-byte signed int */
#endif
#ifndef DREAL                   /* ditto */
#define DREAL double            /* 8-byte floating point */
#endif


#endif /* defined(_GC_MACH_H_) */
