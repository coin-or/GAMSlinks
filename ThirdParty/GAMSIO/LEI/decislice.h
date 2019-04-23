/* licensing utility for use in the decisc executable
 * created Oct 2007, as a spin-off from baronlice.c where it used to be
 * we had to move it so that linking in gcxDecisLice didn't require XPRESS libs
 */

#if ! defined(_DECISLICE_H_)
#define       _DECISLICE_H_


#if defined(_MSC_VER)			/* this is a Visual C compile */
# if ! defined(LICE_CALLCONV)
#  define LICE_CALLCONV __cdecl
# endif
#endif /* #if defined(MSC_VER) j*/

#if ! defined(LICE_CALLCONV)
# define LICE_CALLCONV
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* license call to be made by decis solver.
 * Assumptions:
 *  1. Decis does not initialize the iolib (gfrcnt) in any way
 *
 * Call returns 0 on success, ~0 if you made a calling error
 */
int LICE_CALLCONV
gcxDecisLice (char *controlFile, int *badCplexInit);

#if defined(__cplusplus)
}
#endif

#endif /* if ! defined(_DECISLICE_H_) */
