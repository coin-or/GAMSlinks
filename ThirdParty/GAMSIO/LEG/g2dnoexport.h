/* g2dnoexport.h
 * C headers for mixed-language functions not exported by g2dlib
 * It is assumed the caller will defined one of
 * FNAME_{L|U}CASE_[NO]DECOR to get the right convention
 */

#if ! defined(_G2DNOEXPORT_H_)
#define       _G2DNOEXPORT_H_

#include "g2dexports.h"

#if   defined(FNAME_LCASE_DECOR)        /* f77 names: lower case, trailing _ */
# define G2DGETERRMTD         g2dgeterrmtd_
# define G2DEVALERRMSGX       g2devalerrmsgx_
# define G2DGETFUNERR         g2dgetfunerr_
# define G2DSETFUNERR         g2dsetfunerr_
# define G2DGETROWNUMBERX     g2dgetrownumberx_
# define G2DGETTSTHDR         g2dgettsthdr_
#elif defined(FNAME_LCASE_NODECOR)      /* f77 names: lower case, no _ */
# define G2DGETERRMTD         g2dgeterrmtd
# define G2DEVALERRMSGX       g2devalerrmsgx
# define G2DGETFUNERR         g2dgetfunerr
# define G2DSETFUNERR         g2dsetfunerr
# define G2DGETROWNUMBERX     g2dgetrownumberx
# define G2DGETTSTHDR         g2dgettsthdr
#elif defined(FNAME_UCASE_DECOR)        /* f77 names: upper case, trailing _ */
# define G2DGETERRMTD         G2DGETERRMTD_
# define G2DEVALERRMSGX       G2DEVALERRMSGX_
# define G2DGETFUNERR         G2DGETFUNERR_
# define G2DSETFUNERR         G2DSETFUNERR_
# define G2DGETROWNUMBERX     G2DGETROWNUMBERX_
# define G2DGETTSTHDR         G2DGETTSTHDR_
#elif defined(FNAME_UCASE_NODECOR)	/* f77 names: upper case, no _ */
# define G2DGETERRMTD         G2DGETERRMTD
# define G2DEVALERRMSGX       G2DEVALERRMSGX
# define G2DGETFUNERR         G2DGETFUNERR
# define G2DSETFUNERR         G2DSETFUNERR
# define G2DGETROWNUMBERX     G2DGETROWNUMBERX
# define G2DGETTSTHDR         G2DGETTSTHDR
#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

#define STACKSIZE 130			/* keep in sync with instrManip_mod */

#if defined(__cplusplus)
extern "C" {
#endif

void G2D_CALLCONV
G2DGETERRMTD (int *optVal);

void G2D_CALLCONV
G2DEVALERRMSGX (const int *errNum, const int *rowNum, const double *result);

void G2D_CALLCONV
G2DGETFUNERR (int *optVal);

void G2D_CALLCONV
G2DSETFUNERR (const int *optVal);

int G2D_CALLCONV
G2DGETROWNUMBERX (const unsigned int instr[]);

void G2D_CALLCONV
G2DGETTSTHDR (int *optVal);

#if defined(__cplusplus)
}
#endif

#endif /* #if ! defined(_G2DNOEXPORT_H_) */
