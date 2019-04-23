/*
 * decoration for the mathnew functions produced by p3pc
 * and used in the nonlinear function evaluators (FIOLIB, CIOLIB, G2DLIB)
 */

#if ! defined(_DECOR_H_)
#define       _DECOR_H_

#define G2DMATHNEW_functionvalue     G2DMATHNEWCOMMON.functionvalue
#define G2DMATHNEW_gradientvalue     G2DMATHNEWCOMMON.gradientvalue
#define G2DMATHNEW_hessianvalue      G2DMATHNEWCOMMON.hessianvalue
#define G2DMATHNEW_exceptcode        G2DMATHNEWCOMMON.exceptcode
#define G2DMATHNEW_returncode        G2DMATHNEWCOMMON.returncode
#define G2DMATHNEW_oldstyle          G2DMATHNEWCOMMON.oldstyle
#define G2DMATHNEW_argumentnumbers   G2DMATHNEWCOMMON.nargs
#define G2DMATHNEW_derivativerequest G2DMATHNEWCOMMON.derivrequest
#define CINDX(i,j)               ((j)*G2DMATHNEW_N + (i))

#if   defined(FNAME_LCASE_DECOR) /* fortran names: lower case, trailing _ */
# define CIOMNFMSG               ciomnfmsg_
# define G2DMNFMSG               g2dmnfmsg_
# define FIOMNFMSG               fiomnfmsg_
# define G2DMATHNEW_funceval        F_CALLCONV g2dmnfunceval_
# define G2DMATHNEW_funcinterval    F_CALLCONV g2dmnfuncinterval_
# define G2DMATHNEWCOMMON           g2dmathnewcommon_
# define G2DMNFUNCEVAL              g2dmnfunceval_
# define G2DMNFUNCGETINF            g2dmnfuncgetinf_
# define G2DMNFUNCINTERVAL          g2dmnfuncinterval_
# define G2DMNFUNCNAME              g2dmnfuncname_
# define G2DMNFUNCSMOOTH            g2dmnfuncsmooth_
# define G2DMNFUNCSETINF            g2dmnfuncsetinf_
# define G2DMNFUNCTF                g2dmnfunctf_
#elif defined(FNAME_LCASE_NODECOR) /* fortran names: lower case, no _ */
# define CIOMNFMSG               ciomnfmsg
# define G2DMNFMSG               g2dmnfmsg
# define FIOMNFMSG               fiomnfmsg
# define G2DMATHNEW_funceval        F_CALLCONV g2dmnfunceval
# define G2DMATHNEW_funcinterval    F_CALLCONV g2dmnfuncinterval
# define G2DMATHNEWCOMMON           g2dmathnewcommon
# define G2DMNFUNCEVAL              g2dmnfunceval
# define G2DMNFUNCGETINF            g2dmnfuncgetinf
# define G2DMNFUNCINTERVAL          g2dmnfuncinterval
# define G2DMNFUNCNAME              g2dmnfuncname
# define G2DMNFUNCSMOOTH            g2dmnfuncsmooth
# define G2DMNFUNCSETINF            g2dmnfuncsetinf
# define G2DMNFUNCTF                g2dmnfunctf
#elif defined(FNAME_UCASE_DECOR) /* fortran names: upper case, trailing _ */
# define CIOMNFMSG               CIOMNFMSG_
# define G2DMNFMSG               G2DMNFMSG_
# define FIOMNFMSG               FIOMNFMSG_
# define G2DMATHNEW_funceval        F_CALLCONV G2DMNFUNCEVAL_
# define G2DMATHNEW_funcinterval    F_CALLCONV G2DMNFUNCINTERVAL_
# define G2DMATHNEWCOMMON           G2DMATHNEWCOMMON_
# define G2DMNFUNCEVAL              G2DMNFUNCEVAL_
# define G2DMNFUNCGETINF            G2DMNFUNCGETINF_
# define G2DMNFUNCINTERVAL          G2DMNFUNCINTERVAL_
# define G2DMNFUNCNAME              G2DMNFUNCNAME_
# define G2DMNFUNCSMOOTH            G2DMNFUNCSMOOTH_
# define G2DMNFUNCSETINF            G2DMNFUNCSETINF_
# define G2DMNFUNCTF                G2DMNFUNCTF_
#elif defined(FNAME_UCASE_NODECOR)      /* fortran names: upper case, no _ */
# define CIOMNFMSG               CIOMNFMSG
# define G2DMNFMSG               G2DMNFMSG
# define FIOMNFMSG               FIOMNFMSG
# define G2DMATHNEW_funceval        F_CALLCONV G2DMNFUNCEVAL
# define G2DMATHNEW_funcinterval    F_CALLCONV G2DMNFUNCINTERVAL
# define G2DMATHNEWCOMMON           G2DMATHNEWCOMMON
# define G2DMNFUNCEVAL              G2DMNFUNCEVAL
# define G2DMNFUNCGETINF            G2DMNFUNCGETINF
# define G2DMNFUNCINTERVAL          G2DMNFUNCINTERVAL
# define G2DMNFUNCNAME              G2DMNFUNCNAME
# define G2DMNFUNCSMOOTH            G2DMNFUNCSMOOTH
# define G2DMNFUNCSETINF            G2DMNFUNCSETINF
# define G2DMNFUNCTF                G2DMNFUNCTF
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
G2DMNFUNCGETINF (void);
void F_CALLCONV
G2DMNFUNCNAME (const int *nfunc, int *msgLen, unsigned char *msg, int len);
int F_CALLCONV
G2DMNFUNCSMOOTH (const int *nfunc);
void F_CALLCONV
G2DMNFUNCSETINF (const double *inf);
void F_CALLCONV
G2DMNFUNCTF (const int *nfunc, int *msgLen, unsigned char *msg, int len);

int
G2DMATHNEW_funceval (int *nfunc, double *x);
int
G2DMATHNEW_funcinterval (int *nfunc, double *xmin, double *xmax,
		      double *functionMin, double *functionMax,
		      double *gradientMin, double *gradientMax);
double
G2DMATHNEW_funcgetinf (void);
unsigned char *
G2DMATHNEW_funcname (unsigned char *result, unsigned char rLen, int nfunc);
void
G2DMATHNEW_funcsetinf (double inf);
int
G2DMATHNEW_funcsmooth (int nfunc);
unsigned char *
G2DMATHNEW_functf (unsigned char *result, unsigned char rLen, int nfunc);

#if defined(__cplusplus)
}
#endif
#endif /* ! defined(_DECOR_H_) */
