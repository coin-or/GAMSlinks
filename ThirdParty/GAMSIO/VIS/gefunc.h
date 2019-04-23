/* gefunc.h
 *
 * defines, headers, etc. for the external functions support
 * in a generic IOLIB.
 *
 */

#if ! defined(_GEFUNC_H_)
#define       _GEFUNC_H_

#undef UNIXDLL
#if defined(_WIN32) || defined(_WIN64)
  /* nothing to define */
#else
# define UNIXDLL
#endif

/* F_CALLCONV is the calling convention used by Fortran routines
 * as dictated by the compiler type, flags, etc.  It may be necessary
 * to adjust the C code to the F_CALLCONV accordingly
 */
#if ! defined(F_CALLCONV)
# define F_CALLCONV
#endif

/* GE_CALLCONV is the calling convention used in the DLL/shared object
 * implementing the external functions */
#if defined(GE_CALLCONV)
#  undef GE_CALLCONV
#endif
#if defined(UNIXDLL)
#  define GE_CALLCONV
#else
#  define GE_CALLCONV __stdcall
#endif  /* if defined(UNIXDLL) */

/*     if STR_DESCRIPTOR is defined, Fortran uses a string descriptor
 * elseif STR_MIX_LENGTHS is defined, Fortran mixes lengths with in arglist
 *        (i.e. call foo(name, x) becomes
 *              void foo(char *name, int len, double *x);
 * else   we assume the Unix convention of lengths at the end
 *        (i.e. call foo(name, x) becomes
 *              void foo(char *name, double *x, int len);
 */
#if ! defined(CNAMES) && (defined(WAT) || defined(WPL))
#  define STR_DESCRIPTOR
  typedef struct string_descriptor {
    char *address;
    unsigned length;
  } stringDescriptor;
#else
#  undef  STR_DESCRIPTOR
#endif /* ! defined(CNAMES) && (defined(WAT) || defined(WPL)) */

#if defined(VIS) || defined(ITL)
#  define STR_MIX_LENGTHS
#else
#  undef  STR_MIX_LENGTHS
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(CNAMES)			/* put in some declarations */
int
gfinitdll (const char *DLLNam, int *Icntr, int *rc2);
int
gffiledll (int *Icntr);
int
gfevdll (int *Icntr, double *x, double *f, double *d);
void
gffreedll (int *Icntr);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_GEFUNC_H_) */
