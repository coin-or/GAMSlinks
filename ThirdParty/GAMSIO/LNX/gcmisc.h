/*
 * gcmisc.h
 *
 * these deal with miscellaneous routines
 */

#if ! defined(_GCMISC_H_)
#     define  _GCMISC_H_

#include "gclib.h"

#if defined(__cplusplus)
extern "C" {
#endif

void GC_clearScreen (void);
int GC_dostype (void);
char *
gclstrdup(const char *s);

#if defined(__cplusplus)
}
#endif

#endif /* defined(_GCMISC_H_) */
