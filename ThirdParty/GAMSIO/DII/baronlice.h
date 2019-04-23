/* licensing utility for use in Baron link
 * created 19 July 2001
 * SPD: 27 Aug 2002: added stuff to do DECIS licensing as well
 * SPD: Oct 2007: DECIS stuff moved to decislice.c
 */

#if ! defined(_BARONLICE_H_)
#define       _BARONLICE_H_


#if defined(_MSC_VER)			/* this is a Visual C compile */
# if ! defined(LICE_CALLCONV)
#  define LICE_CALLCONV __cdecl
# endif
#endif /* #if defined(MSC_VER) j*/

#if ! defined(LICE_CALLCONV)
# define LICE_CALLCONV
#endif

#define GCX_DEMO 0
#define GCX_EVAL 1
#define GCX_FULL 2

#define LICINITFAILMASK 0x1
#define BARRIERMASK     0x2
#define MIPMASK         0x4

#if defined(__cplusplus)
extern "C" {
#endif

/* license call to be made by baron solver.
 * Assumptions:
 *  1. Baron does not initialize the iolib (gfrcnt) in any way
 *
 * Call returns 0 on success, ~0 if you made a calling error
 */
int LICE_CALLCONV
gcxBaronLice (char *controlFile,
	      int *mDemo, int *nDemo, int *nzDemo,
	      int *nlnzDemo, int *discDemo,
	      int *permVec, int permVecLen,
	      int *licDetailVec, int licDetailVecLen);

#if defined(__cplusplus)
}
#endif

#endif /* if ! defined(_BARONLICE_H_) */
