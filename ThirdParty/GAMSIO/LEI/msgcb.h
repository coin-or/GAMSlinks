/*
 * defines, headers, etc. for the callback function used in
 * the gefunc() external function call
 *
 * The message callback function is defined in msgcb.c.
 * By including this file and using the normal GAMS build code
 * (e.g -DWAT, -DVIS, -DLNX, etc.), you should be able to refer
 * to the callback symbol simply as MSGCB
 */

#if ! defined(_MSGCB_H_)
#define       _MSGCB_H_

#include "gefunc.h"

#if   defined(CNAMES)
# define MSGCB   msgcb
#elif defined(FNAME_LCASE_DECOR)	/* fortran names: lower case, trailing _ */
# define MSGCB   msgcb_
# define GFPCBK  gfpcbk_
#elif defined(FNAME_LCASE_NODECOR) || defined(CNAMES)/* fortran names: lower case, no _ */
# define MSGCB   msgcb
# define GFPCBK  gfpcbk
#elif defined(FNAME_UCASE_DECOR)	/* fortran names: upper case, trailing _ */
# define MSGCB   MSGCB_
# define GFPCBK  GFPCBK_
#elif defined(FNAME_UCASE_NODECOR)	/* fortran names: upper case, no _ */
# define MSGCB   MSGCB
# define GFPCBK  GFPCBK
#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

/* assumes STR_DESCRIPTOR is already (un)defined
 * and that stringDescriptor is a valid type if needed */


#if ! defined(CNAMES) && (defined(WAT) || defined(WPL))
				/* use string descriptor */
#  pragma aux GFPCBK "^";
#endif

typedef void (GE_CALLCONV * msgcb_t)
     (const int *mode, int *nchars, const char *buf, int len);

#if defined(__cplusplus)
extern "C" {
#endif

void GE_CALLCONV
MSGCB (const int *mode, int *nchars, const char *buf, int len);

#if !defined(CNAMES)
void F_CALLCONV
#  if   defined(STR_DESCRIPTOR)
GFPCBK (stringDescriptor *msg, int *nChars, int *mode);
#  elif defined(STR_MIX_LENGTHS)
GFPCBK (char *msg, int len, int *nChars, int *mode);
#  else  /* the Unix way */
GFPCBK (char *msg, int *nChars, int *mode, int len);
#  endif
#endif /* !defined(CNAMES) */

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_MSGCB_H_) */
