/* gecalls.h
 *
 * Created 28 June 2000: SPD
 *
 * Headers for the C interface calls to the
 * Gams External equation/function routines (init, term, eval).
 */

#if ! defined(_GECALLS_H_)
#define       _GECALLS_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* cioInitDLL
 * Initialize the external function library/DLL
 * output args:
 *   GEm, GEn, GEnz: size stats for the external functions
 * returns:
 *   0 on success
 *  ~0 on error
 */
int
cioInitDLL (int *GEm, int *GEn, int *GEnz);


/* cioEvalDLL
 * Evaluate one of the external functions
 * input args:
 *   GEi:      index of external function in [1..GEm]
 *   gi:       index of external equation in [1..m] - the GAMS row index
 *   doFunc:   perform a function evaluation
 *   doDeriv:  perform a derivative evaluation
 *   isNewPt:  true if x has changed since the last call
 *   GEn:      vars in external function
 *   x:        point at which to evaluate
 * output args:
 *   f:        scalar, LHS in eqn f(GEi) =e= 0
 *   df:       vector, df/dx
 * returns:
 *   0 on success
 *  >0 on error
 */
int
cioEvalDLL (int GEi, int gi, int doFunc, int doDeriv,
	    int isNewPt, int GEn, double x[], double *f, double df[]);

/* cioFreeDLL
 * close down the DLL
 */
void
cioFreeDLL (void);

#if defined(__cplusplus)
}
#endif

#endif /* #if ! defined(_GECALLS_H_) */
