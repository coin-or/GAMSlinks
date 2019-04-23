/*
 * gclib.h
 *
 * types, defines, and headers used to make the
 * C I/O Library machine and compiler-independent.
 *
 * Be careful introducing structures in here; this may
 * require separate libraries on systems where different
 * C products use different alignment.
 */

#if ! defined(_GCLIB_H_)
#     define  _GCLIB_H_

#include <stdio.h>
#include <stdlib.h>
#include "gcmach.h"

#ifndef GC_BOOLEAN
#define GC_BOOLEAN int
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

enum {
  GC_undef,
  GC_text,
  GC_binary,
  GC_read,
  GC_write,
  GC_append
};

enum {
  GC_DOS_NOT = 0,		/* not running DOS */
  GC_DOS_DOS,			/* straight DOS, Win 3.1 (with PharLap) */
  GC_DOS_95,			/* Win95 */
  GC_DOS_NT			/* WinNT */
};

#ifdef NOSTRDUP
char *strdup(const char *st);
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void
gclGetGclInfo (char *srcName, int snlen,
	       int *srcVer,
	       char *srcDate, int sdlen);
void
gclBldInfo (void);

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_GCLIB_H_) */
