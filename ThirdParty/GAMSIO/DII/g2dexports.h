/* g2dexports.h
 * C headers for functions exported by g2dlib
 * It is assumed the caller will defined one of
 * FNAME_{L|U}CASE_[NO]DECOR to get the right convention
 */

#if ! defined(_G2DEXPORTS_H_)
#define       _G2DEXPORTS_H_


/* G2D_CALLCONV is the calling convention used in the DLL/shared object
 * implementing the 2nd derivative functionality */
#if defined(G2D_CALLCONV)
# undef G2D_CALLCONV
#endif

/* G2DCB_CALLCONV is the calling convention used by the callbacks
 * passed to G2DLIB and defined elsewhere */
#if defined(G2DCB_CALLCONV)
# undef G2DCB_CALLCONV
#endif

#if defined(ITL) || defined(WEI)
# define G2D_CALLCONV   __cdecl
# define G2DCB_CALLCONV __stdcall
# if defined(G2D_EXPORTS)
#   define G2D_API __declspec(dllexport)
# else
/* #   define G2D_API __declspec(dllimport) */
/* this is kind of a hack: no DLL-ish decoration unless we say G2D_EXPORTS */
#   define G2D_API
# endif
#elif defined(AXN) || defined(L95)  || defined(VIS) || defined(WAT)
# define G2D_CALLCONV   __stdcall
# define G2DCB_CALLCONV __stdcall
# if defined(G2D_EXPORTS)
#   define G2D_API __declspec(dllexport)
# else
/* #   define G2D_API __declspec(dllimport) */
/* this is kind of a hack: no DLL-ish decoration unless we say G2D_EXPORTS */
#   define G2D_API
# endif
#else
# define G2D_CALLCONV
# define G2DCB_CALLCONV
# define G2D_API
#endif


#if   defined(FNAME_LCASE_DECOR)        /* f77 names: lower case, trailing _ */
# define G2DADDRESSPATCH      g2daddresspatch_
# define G2DCOMPRESSINI       g2dcompressini_
# define G2DCOMPRESSINSTR     g2dcompressinstr_
# define G2DCOMPRESSMEMEST    g2dcompressmemest_
# define G2DCOUNTHESDIAG      g2dcounthesdiag_
# define G2DDIR2D             g2ddir2d_
# define G2DDIR2DX            g2ddir2dx_
# define G2DDUMPEQU           g2ddumpequ_
# define G2DFUNCEVAL0         g2dfunceval0_
# define G2DFUNCEXTRACT       g2dfuncextract_
# define G2DHESLAGSTRUCT      g2dheslagstruct_
# define G2DHESROWSTRUCT      g2dhesrowstruct_
# define G2DHESROWVAL         g2dhesrowval_
# define G2DINIT              g2dinit_
# define G2DINITHESLIST       g2dinitheslist_
# define G2DINITJACLIST       g2dinitjaclist_
# define G2DINSTRREFORM       g2dinstrreform_
# define G2DMERGEHESVAL       g2dmergehesval_
# define G2DNTRI              g2dntri_
# define G2DGETDEBUGHESSTRUCT g2dgetdebughesstruct_
# define G2DGETHESRECSUSEDMAX g2dgethesrecsusedmax_
# define G2DGETHESSPACEGONE   g2dgethesspacegone_
# define G2DGETINSTRTEXT      g2dgetinstrtext_
# define G2DGETPLINFY         g2dgetplinfy_
# define G2DGETREORDERHESSTRUCT g2dgetreorderhesstruct_
# define G2DGETRESSTACKSIZE   g2dgetresstacksize_
# define G2DGRADVPRODEVAL     g2dgradvprodeval_
# define G2DGRADVPRODEVALX    g2dgradvprodevalx_
# define G2DMERGEHESSTRUCT    g2dmergehesstruct_
# define G2DMERGEHESSTRUCTCP  g2dmergehesstructcp_
# define G2DREINITHESLIST     g2dreinitheslist_
# define G2DREINITJACLIST     g2dreinitjaclist_
# define G2DREVERSEEVAL1      g2dreverseeval1_
# define G2DSETDEBUGHESMERGES g2dsetdebughesmerges_
# define G2DSETDEBUGHESSTRUCT g2dsetdebughesstruct_
# define G2DSETDEBUGJACMERGES g2dsetdebugjacmerges_
# define G2DSETDRVMTD         g2dsetdrvmtd_
# define G2DSETERRMTD         g2dseterrmtd_
# define G2DSETMSGCALLBACK    g2dsetmsgcallback_
# define G2DSETPR2VAL         g2dsetpr2val_
# define G2DSETPR2DRV         g2dsetpr2drv_
# define G2DSETPRDERV         g2dsetprderv_
# define G2DSETPRINSTR        g2dsetprinstr_
# define G2DSETPRHSTR         g2dsetprhstr_
# define G2DSETPRNTR2         g2dsetprntr2_
# define G2DSETPRNTRI         g2dsetprntri_
# define G2DSETPRNTRP         g2dsetprntrp_
# define G2DSETREFORM         g2dsetreform_
# define G2DSETREORDERHESSTRUCT g2dsetreorderhesstruct_
# define G2DSETSLINFY         g2dsetslinfy_
# define G2DSETTSTHDR         g2dsettsthdr_
# define G2DSTATSINIT         g2dstatsinit_
#elif defined(FNAME_LCASE_NODECOR)      /* f77 names: lower case, no _ */
# define G2DADDRESSPATCH      g2daddresspatch
# define G2DCOMPRESSINI       g2dcompressini
# define G2DCOMPRESSINSTR     g2dcompressinstr
# define G2DCOMPRESSMEMEST    g2dcompressmemest
# define G2DCOUNTHESDIAG      g2dcounthesdiag
# define G2DDIR2D             g2ddir2d
# define G2DDIR2DX            g2ddir2dx
# define G2DDUMPEQU           g2ddumpequ
# define G2DFUNCEVAL0         g2dfunceval0
# define G2DFUNCEXTRACT       g2dfuncextract
# define G2DHESLAGSTRUCT      g2dheslagstruct
# define G2DHESROWSTRUCT      g2dhesrowstruct
# define G2DHESROWVAL         g2dhesrowval
# define G2DINIT              g2dinit
# define G2DINITHESLIST       g2dinitheslist
# define G2DINITJACLIST       g2dinitjaclist
# define G2DINSTRREFORM       g2dinstrreform
# define G2DMERGEHESVAL       g2dmergehesval
# define G2DNTRI              g2dntri
# define G2DGETDEBUGHESSTRUCT g2dgetdebughesstruct
# define G2DGETHESRECSUSEDMAX g2dgethesrecsusedmax
# define G2DGETHESSPACEGONE   g2dgethesspacegone
# define G2DGETINSTRTEXT      g2dgetinstrtext
# define G2DGETPLINFY         g2dgetplinfy
# define G2DGETREORDERHESSTRUCT g2dgetreorderhesstruct
# define G2DGETRESSTACKSIZE   g2dgetresstacksize
# define G2DGRADVPRODEVAL     g2dgradvprodeval
# define G2DGRADVPRODEVALX    g2dgradvprodevalx
# define G2DMERGEHESSTRUCT    g2dmergehesstruct
# define G2DMERGEHESSTRUCTCP  g2dmergehesstructcp
# define G2DREINITHESLIST     g2dreinitheslist
# define G2DREINITJACLIST     g2dreinitjaclist
# define G2DREVERSEEVAL1      g2dreverseeval1
# define G2DSETDEBUGHESMERGES g2dsetdebughesmerges
# define G2DSETDEBUGHESSTRUCT g2dsetdebughesstruct
# define G2DSETDEBUGJACMERGES g2dsetdebugjacmerges
# define G2DSETDRVMTD         g2dsetdrvmtd
# define G2DSETERRMTD         g2dseterrmtd
# define G2DSETMSGCALLBACK    g2dsetmsgcallback
# define G2DSETPR2DRV         g2dsetpr2drv
# define G2DSETPR2VAL         g2dsetpr2val
# define G2DSETPRDERV         g2dsetprderv
# define G2DSETPRINSTR        g2dsetprinstr
# define G2DSETPRHSTR         g2dsetprhstr
# define G2DSETPRNTR2         g2dsetprntr2
# define G2DSETPRNTRI         g2dsetprntri
# define G2DSETPRNTRP         g2dsetprntrp
# define G2DSETREFORM         g2dsetreform
# define G2DSETREORDERHESSTRUCT g2dsetreorderhesstruct
# define G2DSETSLINFY         g2dsetslinfy
# define G2DSETTSTHDR         g2dsettsthdr
# define G2DSTATSINIT         g2dstatsinit
#elif defined(FNAME_UCASE_DECOR)        /* f77 names: upper case, trailing _ */
# define G2DADDRESSPATCH      G2DADDRESSPATCH_
# define G2DCOMPRESSINI       G2DCOMPRESSINI_
# define G2DCOMPRESSINSTR     G2DCOMPRESSINSTR_
# define G2DCOMPRESSMEMEST    G2DCOMPRESSMEMEST_
# define G2DCOUNTHESDIAG      G2DCOUNTHESDIAG_
# define G2DDIR2D             G2DDIR2D_
# define G2DDIR2DX            G2DDIR2DX_
# define G2DDUMPEQU           G2DDUMPEQU_
# define G2DFUNCEVAL0         G2DFUNCEVAL0_
# define G2DFUNCEXTRACT       G2DFUNCEXTRACT_
# define G2DHESLAGSTRUCT      G2DHESLAGSTRUCT_
# define G2DHESROWSTRUCT      G2DHESROWSTRUCT_
# define G2DHESROWVAL         G2DHESROWVAL_
# define G2DINIT              G2DINIT_
# define G2DINITHESLIST       G2DINITHESLIST_
# define G2DINITJACLIST       G2DINITJACLIST_
# define G2DINSTRREFORM       G2DINSTRREFORM_
# define G2DMERGEHESVAL       G2DMERGEHESVAL_
# define G2DNTRI              G2DNTRI_
# define G2DGETDEBUGHESSTRUCT G2DGETDEBUGHESSTRUCT_
# define G2DGETHESRECSUSEDMAX G2DGETHESRECSUSEDMAX_
# define G2DGETHESSPACEGONE   G2DGETHESSPACEGONE_
# define G2DGETINSTRTEXT      G2DGETINSTRTEXT_
# define G2DGETPLINFY         G2DGETPLINFY_
# define G2DGETREORDERHESSTRUCT G2DGETREORDERHESSTRUCT_
# define G2DGETRESSTACKSIZE   G2DGETRESSTACKSIZE_
# define G2DGRADVPRODEVAL     G2DGRADVPRODEVAL_
# define G2DGRADVPRODEVALX    G2DGRADVPRODEVALX_
# define G2DMERGEHESSTRUCT    G2DMERGEHESSTRUCT_
# define G2DMERGEHESSTRUCTCP  G2DMERGEHESSTRUCTCP_
# define G2DREINITHESLIST     G2DREINITHESLIST_
# define G2DREINITJACLIST     G2DREINITJACLIST_
# define G2DREVERSEEVAL1      G2DREVERSEEVAL1_
# define G2DSETDEBUGHESMERGES G2DSETDEBUGHESMERGES_
# define G2DSETDEBUGHESSTRUCT G2DSETDEBUGHESSTRUCT_
# define G2DSETDEBUGJACMERGES G2DSETDEBUGJACMERGES_
# define G2DSETDRVMTD         G2DSETDRVMTD_
# define G2DSETERRMTD         G2DSETERRMTD_
# define G2DSETMSGCALLBACK    G2DSETMSGCALLBACK_
# define G2DSETPR2DRV         G2DSETPR2DRV_
# define G2DSETPR2VAL         G2DSETPR2VAL_
# define G2DSETPRDERV         G2DSETPRDERV_
# define G2DSETPRINSTR        G2DSETPRINSTR_
# define G2DSETPRHSTR         G2DSETPRHSTR_
# define G2DSETPRNTR2         G2DSETPRNTR2_
# define G2DSETPRNTRI         G2DSETPRNTRI_
# define G2DSETPRNTRP         G2DSETPRNTRP_
# define G2DSETREFORM         G2DSETREFORM_
# define G2DSETREORDERHESSTRUCT G2DSETREORDERHESSTRUCT_
# define G2DSETSLINFY         G2DSETSLINFY_
# define G2DSETTSTHDR         G2DSETTSTHDR_
# define G2DSTATSINIT         G2DSTATSINIT_
#elif defined(FNAME_UCASE_NODECOR)	/* f77 names: upper case, no _ */
# define G2DADDRESSPATCH      G2DADDRESSPATCH
# define G2DCOMPRESSINI       G2DCOMPRESSINI
# define G2DCOMPRESSINSTR     G2DCOMPRESSINSTR
# define G2DCOMPRESSMEMEST    G2DCOMPRESSMEMEST
# define G2DCOUNTHESDIAG      G2DCOUNTHESDIAG
# define G2DDIR2D             G2DDIR2D
# define G2DDIR2DX            G2DDIR2DX
# define G2DDUMPEQU           G2DDUMPEQU
# define G2DFUNCEVAL0         G2DFUNCEVAL0
# define G2DFUNCEXTRACT       G2DFUNCEXTRACT
# define G2DHESLAGSTRUCT      G2DHESLAGSTRUCT
# define G2DHESROWSTRUCT      G2DHESROWSTRUCT
# define G2DHESROWVAL         G2DHESROWVAL
# define G2DINIT              G2DINIT
# define G2DINITHESLIST       G2DINITHESLIST
# define G2DINITJACLIST       G2DINITJACLIST
# define G2DINSTRREFORM       G2DINSTRREFORM
# define G2DMERGEHESVAL       G2DMERGEHESVAL
# define G2DNTRI              G2DNTRI
# define G2DGETDEBUGHESSTRUCT G2DGETDEBUGHESSTRUCT
# define G2DGETHESRECSUSEDMAX G2DGETHESRECSUSEDMAX
# define G2DGETHESSPACEGONE   G2DGETHESSPACEGONE
# define G2DGETINSTRTEXT      G2DGETINSTRTEXT
# define G2DGETPLINFY         G2DGETPLINFY
# define G2DGETREORDERHESSTRUCT G2DGETREORDERHESSTRUCT
# define G2DGETRESSTACKSIZE   G2DGETRESSTACKSIZE
# define G2DGRADVPRODEVAL     G2DGRADVPRODEVAL
# define G2DGRADVPRODEVALX    G2DGRADVPRODEVALX
# define G2DMERGEHESSTRUCT    G2DMERGEHESSTRUCT
# define G2DMERGEHESSTRUCTCP  G2DMERGEHESSTRUCTCP
# define G2DREINITHESLIST     G2DREINITHESLIST
# define G2DREINITJACLIST     G2DREINITJACLIST
# define G2DREVERSEEVAL1      G2DREVERSEEVAL1
# define G2DSETDEBUGHESMERGES G2DSETDEBUGHESMERGES
# define G2DSETDEBUGHESSTRUCT G2DSETDEBUGHESSTRUCT
# define G2DSETDEBUGJACMERGES G2DSETDEBUGJACMERGES
# define G2DSETDRVMTD         G2DSETDRVMTD
# define G2DSETERRMTD         G2DSETERRMTD
# define G2DSETMSGCALLBACK    G2DSETMSGCALLBACK
# define G2DSETPR2DRV         G2DSETPR2DRV
# define G2DSETPR2VAL         G2DSETPR2VAL
# define G2DSETPRDERV         G2DSETPRDERV
# define G2DSETPRINSTR        G2DSETPRINSTR
# define G2DSETPRHSTR         G2DSETPRHSTR
# define G2DSETPRNTR2         G2DSETPRNTR2
# define G2DSETPRNTRI         G2DSETPRNTRI
# define G2DSETPRNTRP         G2DSETPRNTRP
# define G2DSETREFORM         G2DSETREFORM
# define G2DSETREORDERHESSTRUCT G2DSETREORDERHESSTRUCT
# define G2DSETSLINFY         G2DSETSLINFY
# define G2DSETTSTHDR         G2DSETTSTHDR
# define G2DSTATSINIT         G2DSTATSINIT
#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

typedef void (G2DCB_CALLCONV * g2dmsgcb_t)
     (const int *mode, int *nchars, const char *buf, int len);
typedef void (G2DCB_CALLCONV * g2dstatsInit_t)
     (int *, int *);

#if defined(__cplusplus)
extern "C" {
#endif

void G2D_CALLCONV
G2DADDRESSPATCH (unsigned int instr[], const int *numIns, int *check);
void G2D_CALLCONV
G2DCOMPRESSINI (int comp[], const int *lenComp, int *cini);
void G2D_CALLCONV
G2DCOMPRESSINSTR (unsigned int instr[], const int *numIns,
		  int *newLength, int comp[], int *cini,
		  double nlCons[], const int *lenNlCons);
void G2D_CALLCONV
G2DCOMPRESSMEMEST (int *lenComp);
void G2D_CALLCONV
G2DCOUNTHESDIAG (int *lagPtr, int *nDiag,
		 int hesCL[], int hesRW[], int hesNX[]);
void G2D_CALLCONV
G2DDIR2D (double x[], double dx[], double wx[], double *g, double *dg,
	  double v[], double vdot[], double vbar[], double vdotbar[],
	  unsigned int instr[], double nlCons[], int *isErr);
void G2D_CALLCONV
G2DDIR2DX (double x[], double dx[], double wx[], double *g, double *dg,
	   double resStack[], unsigned int instr[], const int *numIns,
	   double nlCons[], int *isErr);
void G2D_CALLCONV
G2DDUMPEQU (const int *channels, const unsigned int instr[]);
G2D_API void G2D_CALLCONV
G2DFUNCEVAL0 (const double x[], double *f, double v[],
	      unsigned int instr[], const int *numIns,
	      double nlCons[], double *rowScale, int *numErr);
void G2D_CALLCONV
G2DFUNCEXTRACT (unsigned int fginstr[], unsigned int *finstr,
		const int *lenIns, int *funcOnlyLen);
G2D_API void G2D_CALLCONV
G2DHESLAGSTRUCT (int jacVR[], int jacNX[], int *lenJac,
		 int hesCL[], int hesRW[], int hesNX[], int *lenHes,
		 int startIdx[], unsigned int instr[], const int *includeObj,
		 int *lagPtr, int *lagLen,
		 int *lenHesMin, int *lenLagMin, int *lenJacMin,
		 int *shortSpace, int *lastEqu, int *denseEqu);
G2D_API void G2D_CALLCONV
G2DHESROWSTRUCT (unsigned int instr[],
		 int *hesHeadPtr, int *hesTailPtr, int *hesNumElem,
		 int *jacHeadPtr, int *jacTailPtr, int *jacNumElem,
		 int jacVR[], int jacNX[],
		 int hesCL[], int hesRW[], int hesNX[], int *extraMemMax);
G2D_API void G2D_CALLCONV
G2DHESROWVAL (const double x[], double *f, unsigned int instr[],
	      double nlCons[], int *numErr,
	      int *hesHeadPtr, int *hesTailPtr, int *hesNumElem,
	      int *jacHeadPtr, int *jacTailPtr, int *jacNumElem,
	      int jacVR[], int jacNX[], double jacVL[],
	      int hesCL[], int hesRW[], int hesNX[], double hesVL[]);
G2D_API void G2D_CALLCONV
G2DINIT (void);
G2D_API void G2D_CALLCONV
G2DINITHESLIST (int hesNX[], int *lenHes);
G2D_API void G2D_CALLCONV
G2DINITJACLIST (int jacNX[], int *lenJac);
G2D_API void G2D_CALLCONV
G2DINSTRREFORM (int eqnStarts[], int eqnLens[], unsigned int instr1[],
		const int *objRow, const int *objCol);
G2D_API void G2D_CALLCONV
G2DMERGEHESVAL (int *headPtrNew, int *tailPtrNew, int *numElemNew,
		int *headPtrOld, int *tailPtrOld, int *numElemOld,
		int colIdx[], int rowIdx[], int next[], double val[]);
void G2D_CALLCONV
G2DNTRI (double xMin[], double xMax[], double *gMin, double *gMax,
	 double sMin[], double sMax[], unsigned int instr[],
	 double nlCons[], double *rowScale);

G2D_API void G2D_CALLCONV
G2DGETDEBUGHESSTRUCT (int *optVal);
G2D_API void G2D_CALLCONV
G2DGETHESRECSUSEDMAX (int *optVal);
G2D_API void G2D_CALLCONV
G2DGETHESSPACEGONE (int *optVal);

void G2D_CALLCONV
G2DGETINSTRTEXT (const int *opCode, int *nChars, char *buf, int len);

G2D_API void G2D_CALLCONV
G2DGETPLINFY (double *val);
G2D_API void G2D_CALLCONV
G2DGETREORDERHESSTRUCT (int *optVal);
int G2D_CALLCONV
G2DGETRESSTACKSIZE(unsigned int instr[], const int *numIns);
void G2D_CALLCONV
G2DGRADVPRODEVAL (double x[], double d[], double *f, double *df_d,
		  double v[], double vdot[],
		  unsigned int instr[], const int *numIns,
		  double c[], int *numErr);

void G2D_CALLCONV
G2DGRADVPRODEVALX (double x[], double d[], double *f, double *df_d,
		   unsigned int instr[], const int *numIns,
		   double c[], int *numErr);
void G2D_CALLCONV
G2DMERGEHESSTRUCT (const int hesCol[], const int hesRow[], const int hesNext[],
		   const double hesVal[], int headPtr, int tailPtr, int len,
		   double lam,
		   const int lagCol[], const int lagRow[], double lagVal[],
		   int lagLen);
void G2D_CALLCONV
G2DMERGEHESSTRUCTCP (const int hesCol[], const int hesRow[],
		     const int hesNext[], const double hesVal[],
		     int headPtr, int tailPtr, int len,
		     double lam,
		     const int lagCol[], const int lagRow[],
		     double lagVal[], int lagLen, const int lagColPtr[]);
G2D_API void G2D_CALLCONV
G2DREINITHESLIST (int hesNX[], int *lenHes);
G2D_API void G2D_CALLCONV
G2DREINITJACLIST (int jacNX[], int *lenJac);

G2D_API void G2D_CALLCONV
G2DREVERSEEVAL1 (double x[], double *dfdx, double v[], double vbar[],
		 unsigned int instr[], const int *numIns,
		 double nlCons[], double *rowScale, int *numErr);

G2D_API void G2D_CALLCONV
G2DSETDEBUGHESMERGES (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETDEBUGHESSTRUCT (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETDEBUGJACMERGES (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETDRVMTD (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETERRMTD (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETMSGCALLBACK (g2dmsgcb_t ptr);
G2D_API void G2D_CALLCONV
G2DSETPR2DRV (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPR2VAL (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRDERV (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRINSTR (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRHSTR (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRNTR2 (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRNTRI (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETPRNTRP (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETREFORM (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETREORDERHESSTRUCT (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSETSLINFY (const double *optVal);
G2D_API void G2D_CALLCONV
G2DSETTSTHDR (const int *optVal);
G2D_API void G2D_CALLCONV
G2DSTATSINIT (g2dstatsInit_t ptr);

#if defined(__cplusplus)
}
#endif

#endif /* #if ! defined(_G2DEXPORTS_H_) */
