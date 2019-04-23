/*
 * decoration for the mathnew functions produced by p3pc
 * and used in the nonlinear function evaluators (FIOLIB, CIOLIB, G2DLIB)
 */

#if ! defined(_DECOR_H_)
#define       _DECOR_H_

#define CIOMATHNEW_functionvalue     CIOMATHNEWCOMMON.functionvalue
#define CIOMATHNEW_gradientvalue     CIOMATHNEWCOMMON.gradientvalue
#define CIOMATHNEW_hessianvalue      CIOMATHNEWCOMMON.hessianvalue
#define CIOMATHNEW_exceptcode        CIOMATHNEWCOMMON.exceptcode
#define CIOMATHNEW_returncode        CIOMATHNEWCOMMON.returncode
#define CIOMATHNEW_oldstyle          CIOMATHNEWCOMMON.oldstyle
#define CIOMATHNEW_argumentnumbers   CIOMATHNEWCOMMON.nargs
#define CIOMATHNEW_derivativerequest CIOMATHNEWCOMMON.derivrequest
#define CINDX(i,j)               ((j)*CIOMATHNEW_N + (i))

#if   defined(FNAME_LCASE_DECOR) /* fortran names: lower case, trailing _ */
# define CIOMNFMSG               ciomnfmsg_
# define G2DMNFMSG               g2dmnfmsg_
# define FIOMNFMSG               fiomnfmsg_
# define CIOMATHNEW_funceval        F_CALLCONV ciomnfunceval_
# define CIOMATHNEW_funcinterval    F_CALLCONV ciomnfuncinterval_
# define CIOMATHNEWCOMMON           ciomathnewcommon_
# define CIOMNFUNCEVAL              ciomnfunceval_
# define CIOMNFUNCGETINF            ciomnfuncgetinf_
# define CIOMNFUNCINTERVAL          ciomnfuncinterval_
# define CIOMNFUNCNAME              ciomnfuncname_
# define CIOMNFUNCSMOOTH            ciomnfuncsmooth_
# define CIOMNFUNCSETINF            ciomnfuncsetinf_
# define CIOMNFUNCTF                ciomnfunctf_
#elif defined(FNAME_LCASE_NODECOR) /* fortran names: lower case, no _ */
# define CIOMNFMSG               ciomnfmsg
# define G2DMNFMSG               g2dmnfmsg
# define FIOMNFMSG               fiomnfmsg
# define CIOMATHNEW_funceval        F_CALLCONV ciomnfunceval
# define CIOMATHNEW_funcinterval    F_CALLCONV ciomnfuncinterval
# define CIOMATHNEWCOMMON           ciomathnewcommon
# define CIOMNFUNCEVAL              ciomnfunceval
# define CIOMNFUNCGETINF            ciomnfuncgetinf
# define CIOMNFUNCINTERVAL          ciomnfuncinterval
# define CIOMNFUNCNAME              ciomnfuncname
# define CIOMNFUNCSMOOTH            ciomnfuncsmooth
# define CIOMNFUNCSETINF            ciomnfuncsetinf
# define CIOMNFUNCTF                ciomnfunctf
#elif defined(FNAME_UCASE_DECOR) /* fortran names: upper case, trailing _ */
# define CIOMNFMSG               CIOMNFMSG_
# define G2DMNFMSG               G2DMNFMSG_
# define FIOMNFMSG               FIOMNFMSG_
# define CIOMATHNEW_funceval        F_CALLCONV CIOMNFUNCEVAL_
# define CIOMATHNEW_funcinterval    F_CALLCONV CIOMNFUNCINTERVAL_
# define CIOMATHNEWCOMMON           CIOMATHNEWCOMMON_
# define CIOMNFUNCEVAL              CIOMNFUNCEVAL_
# define CIOMNFUNCGETINF            CIOMNFUNCGETINF_
# define CIOMNFUNCINTERVAL          CIOMNFUNCINTERVAL_
# define CIOMNFUNCNAME              CIOMNFUNCNAME_
# define CIOMNFUNCSMOOTH            CIOMNFUNCSMOOTH_
# define CIOMNFUNCSETINF            CIOMNFUNCSETINF_
# define CIOMNFUNCTF                CIOMNFUNCTF_
#elif defined(FNAME_UCASE_NODECOR)      /* fortran names: upper case, no _ */
# define CIOMNFMSG               CIOMNFMSG
# define G2DMNFMSG               G2DMNFMSG
# define FIOMNFMSG               FIOMNFMSG
# define CIOMATHNEW_funceval        F_CALLCONV CIOMNFUNCEVAL
# define CIOMATHNEW_funcinterval    F_CALLCONV CIOMNFUNCINTERVAL
# define CIOMATHNEWCOMMON           CIOMATHNEWCOMMON
# define CIOMNFUNCEVAL              CIOMNFUNCEVAL
# define CIOMNFUNCGETINF            CIOMNFUNCGETINF
# define CIOMNFUNCINTERVAL          CIOMNFUNCINTERVAL
# define CIOMNFUNCNAME              CIOMNFUNCNAME
# define CIOMNFUNCSMOOTH            CIOMNFUNCSMOOTH
# define CIOMNFUNCSETINF            CIOMNFUNCSETINF
# define CIOMNFUNCTF                CIOMNFUNCTF
#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

/* F_CALLCONV is the calling convention used by Fortran routines
 * as dictated by the compiler type, flags, etc.  It may be necessary
 * to adjust the C code to fit the prevailing F_CALLCONV
 */
#if ! defined(F_CALLCONV)
# define F_CALLCONV
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void F_CALLCONV
CIOMNFMSG (int *msgLen, unsigned char *msg, int len);
void F_CALLCONV
G2DMNFMSG (int *msgLen, unsigned char *msg, int len);
void F_CALLCONV
FIOMNFMSG (int *msgLen, unsigned char *msg, int len);

double F_CALLCONV
CIOMNFUNCGETINF (void);
void F_CALLCONV
CIOMNFUNCNAME (const int *nfunc, int *msgLen, unsigned char *msg, int len);
int F_CALLCONV
CIOMNFUNCSMOOTH (const int *nfunc);
void F_CALLCONV
CIOMNFUNCSETINF (const double *inf);
void F_CALLCONV
CIOMNFUNCTF (const int *nfunc, int *msgLen, unsigned char *msg, int len);

int
CIOMATHNEW_funceval (int *nfunc, double *x);
int
CIOMATHNEW_funcinterval (int *nfunc, double *xmin, double *xmax,
		      double *functionMin, double *functionMax,
		      double *gradientMin, double *gradientMax);
double
CIOMATHNEW_funcgetinf (void);
unsigned char *
CIOMATHNEW_funcname (unsigned char *result, unsigned char rLen, int nfunc);
void
CIOMATHNEW_funcsetinf (double inf);
int
CIOMATHNEW_funcsmooth (int nfunc);
unsigned char *
CIOMATHNEW_functf (unsigned char *result, unsigned char rLen, int nfunc);

#if defined(__cplusplus)
}
#endif
#endif /* ! defined(_DECOR_H_) */
