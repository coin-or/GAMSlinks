#if ! defined(_G2DCMSGS_H_)
#define       _G2DCMSGS_H_

/* F_CALLCONV is the calling convention used by Fortran routines
 * as dictated by the compiler type, flags, etc.  It may be necessary
 * to adjust the C code to the F_CALLCONV accordingly
 */
#if ! defined(F_CALLCONV)
# define F_CALLCONV
#endif

/*     if STR_DESCRIPTOR is defined, Fortran uses a string descriptor
 * elseif STR_MIX_LENGTHS is defined, Fortran mixes lengths with in arglist
 *        (i.e. call foo(name, x) becomes
 *              void foo(char *name, int len, double *x);
 * else   we assume the Unix convention of lengths at the end
 *        (i.e. call foo(name, x) becomes
 *              void foo(char *name, double *x, int len);
 */
#if defined(WAT) || defined(WPL)
#  define STR_DESCRIPTOR
  typedef struct string_descriptor {
    char *address;
    unsigned length;
  } stringDescriptor;
#else
#  undef  STR_DESCRIPTOR
#endif /* defined(WAT) || defined(WPL) */

#if defined(VIS)
#  define STR_MIX_LENGTHS
#else
#  undef  STR_MIX_LENGTHS
#endif

#if   defined(FNAME_LCASE_DECOR)        /* f77 names: lower case, trailing _ */
# define G2DMSGWRAPPER        g2dmsgwrapper_
#elif defined(FNAME_LCASE_NODECOR)	/* f77 names: lower case, no _ */
# define G2DMSGWRAPPER        g2dmsgwrapper
#elif defined(FNAME_UCASE_DECOR)        /* f77 names: upper case, trailing _ */
# define G2DMSGWRAPPER        G2DMSGWRAPPER_
#elif defined(FNAME_UCASE_NODECOR)      /* f77 names: upper case, no _ */
# define G2DMSGWRAPPER        G2DMSGWRAPPER
#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

extern g2dmsgcb_t g2dmsgcbPtr;

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_G2DCMSGS_H_) */
