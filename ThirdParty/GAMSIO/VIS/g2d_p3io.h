#ifndef _MYP3IO_H
#define _MYP3IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef  signed char SYSTEM_boolean;
#define SYSTEM_false ((SYSTEM_boolean)0)
#define SYSTEM_true  ((SYSTEM_boolean)1)

/* Declare the fundamental types: SYSTEM_[u_]int{8|16|32},
   the 32 versions different on 64-bit systems (alpha) and the others */
typedef  signed char    SYSTEM_int8;   /* Use 'short' if errors */
typedef  short          SYSTEM_int16;
typedef  unsigned char  SYSTEM_uint8;
typedef  unsigned short SYSTEM_uint16;
typedef  int            SYSTEM_int32;
typedef  unsigned int   SYSTEM_uint32;

/* The following implement the integer type declarations in system.pas,
   based on the "fundamental" types: SYSTEM_[u_]int{8|16|32}          */
typedef SYSTEM_int32    SYSTEM_longint;
typedef SYSTEM_int16    SYSTEM_smallint;
typedef SYSTEM_int8     SYSTEM_shortint;
typedef SYSTEM_uint32   SYSTEM_longword;
typedef SYSTEM_uint16   SYSTEM_word;
typedef SYSTEM_uint8    SYSTEM_byte;

typedef  SYSTEM_longint  SYSTEM_integer;
typedef  unsigned char  SYSTEM_char;       /* Keep unsigned so ord works!! */
typedef  float          SYSTEM_single;
typedef  double         SYSTEM_double;
typedef  SYSTEM_double  SYSTEM_real;       /* Double/real are synonyms */
typedef void            *SYSTEM_pointer; /* Can point to anything */
typedef SYSTEM_char     SYSTEM_shortstring[256]; /* Used in system.pas */

#define _P3bits_per_byte 8 /* Don't run on machines with smaller bytes */
#define _P3setsize(i) ((i)/_P3bits_per_elem+1) /* bytes to hold 0..i bits */
#define _P3bits_per_elem  (_P3bits_per_byte*sizeof(_P3set_elem))
#define _P3set_max   (256/_P3bits_per_elem) /* Length of set in elem's */
typedef SYSTEM_char     _P3set_elem;
typedef _P3set_elem     *_P3Tset_ptr;
typedef _P3set_elem     _P3set255[_P3setsize(255)];

/* To make proc/func declarations look a little nicer (?): */
#define Procedure void
#define Function(t) t
#define Prototype typedef
#define cnstdef enum
#define ValueCast(t,e)     (t)(e)
#define SYSTEM_break(L)    goto L
#define SYSTEM_continue(L) goto L
#define _P3char(c)  ((SYSTEM_char)(c)) /* Otherwise '\377' is negative */
#define SYSTEM_maxint     2147483647 /* 2**31-1 */
#define SYSTEM_maxlongint 2147483647 /* 2**31-1 */

#define _P3str1(s)  ((SYSTEM_char*)(s))
#define _P3strcat(a,max,b,c) g2dP3_strcat(a,max,b,c)
#define _P3strcpy(d,max,s)   g2dP3_strcpy(d,max,s)
#define _P3inc0(x)       ++(x)
#define _P3inc1(x,i)   (x)+=(i)
#define _P3dec0(x)       --(x)
#define _P3dec1(x,i)   (x)-=(i)
#define SYSTEM_noassert(b,w)  /* ignore assertion */
#define SYSTEM_odd(x)         ((x)&1)
#define SYSTEM_abs_i(x) abs((SYSTEM_int32)(x))
#define SYSTEM_abs_r(x) fabs((double)(x))
#define SYSTEM_sin(x)         sin((double)(x))
#define SYSTEM_cos(x)         cos((double)(x))
#define SYSTEM_arctan(x)      atan((double)(x))
#define SYSTEM_sqrt(x)        sqrt((double)(x))
#define SYSTEM_exp(x)         exp((double)(x))
#define SYSTEM_ln(x)          log((double)(x))
#define SYSTEM_chr(x)         ((SYSTEM_byte)(x))
#define SYSTEM_trunc(x)       (long)(x)

extern SYSTEM_char *g2dP3_strcat(SYSTEM_char *d,  SYSTEM_char max,
                               SYSTEM_char *p1, SYSTEM_char *p2);
extern SYSTEM_byte *g2dP3_strcpy(SYSTEM_byte *d,  SYSTEM_integer max,
			       SYSTEM_byte *s);
extern SYSTEM_double  g2dSYSTEM_sqr_r(SYSTEM_double x);
extern SYSTEM_double  g2dSYSTEM_int(SYSTEM_double x);
extern SYSTEM_double  g2dSYSTEM_frac(SYSTEM_double x);
extern SYSTEM_longint g2dSYSTEM_round(SYSTEM_double x);
extern void g2dP3setlength(SYSTEM_byte *s, SYSTEM_integer l, SYSTEM_integer m);
#define SYSTEM_ord(x)          (x)

/* Next 4 used to inline smart 'in' tests: */
#define _P3set_Equal(i,j)     ((i) == (j))
#define _P3set_in_1(i,j,k)    (((i) == (j)) || (k))
#define _P3set_in_2(i,j,k,l)  (((i) >= (j) && (i) <= (k)) || (l))
#define _P3set_in_3(i,j,k)    ((i) >= (j) && (i) <= (k))

/* STRINGS */

typedef SYSTEM_char _P3STR_3[4];
typedef SYSTEM_char _P3STR_7[8];
typedef SYSTEM_char _P3STR_15[16];
typedef SYSTEM_char _P3STR_31[32];
typedef SYSTEM_char _P3STR_63[64];
typedef SYSTEM_char _P3STR_95[96];
typedef SYSTEM_char _P3STR_127[128];
typedef SYSTEM_char _P3STR_159[160];
typedef SYSTEM_char _P3STR_191[192];
typedef SYSTEM_char _P3STR_223[224];
typedef SYSTEM_char _P3STR_255[256];
typedef SYSTEM_char _P3SET_3[_P3setsize(3)];
typedef SYSTEM_char _P3SET_7[_P3setsize(7)];
typedef SYSTEM_char _P3SET_15[_P3setsize(15)];
typedef SYSTEM_char _P3SET_31[_P3setsize(31)];
typedef SYSTEM_char _P3SET_63[_P3setsize(63)];
typedef SYSTEM_char _P3SET_95[_P3setsize(95)];
typedef SYSTEM_char _P3SET_127[_P3setsize(127)];
typedef SYSTEM_char _P3SET_159[_P3setsize(159)];
typedef SYSTEM_char _P3SET_191[_P3setsize(191)];
typedef SYSTEM_char _P3SET_223[_P3setsize(223)];
typedef SYSTEM_char _P3SET_255[_P3setsize(255)];

/* now some headers from MATH_P3 unit */

#define g2dMATH_P3_arccos(x) acos(x)
Function(SYSTEM_double ) g2dMATH_P3_arcsin(SYSTEM_double x);
Function(SYSTEM_double ) g2dMATH_P3_arctan2(SYSTEM_double y,SYSTEM_double x);
Function(SYSTEM_double ) g2dMATH_P3_tan(SYSTEM_double x);

#endif /* ifdef _MYP3IO_H */
