/* smag.h
 * public header file for SMAG IO library
 */

#if ! defined(_SMAG_H_)
#     define  _SMAG_H_

#include <stdio.h>
#include <stdarg.h>

#define OBJ_FUNCTION 322
#define OBJ_VARIABLE 323

#define SMAG_EQU_EQ 0
#define SMAG_EQU_GT 1
#define SMAG_EQU_LT 2
#define SMAG_EQU_NB 3
#define SMAG_EQU_XE 4
#define SMAG_EQU_CE 5
#define SMAG_NROWTYPES (SMAG_EQU_CE+1)

#define SMAG_VAR_CONT     0
#define SMAG_VAR_BINARY   1
#define SMAG_VAR_INTEGER  2
#define SMAG_VAR_SOS1     3
#define SMAG_VAR_SOS2     4
#define SMAG_VAR_SEMICONT 5
#define SMAG_VAR_SEMIINT  6

#define SMAG_BASSTAT_NBLOWER    0
#define SMAG_BASSTAT_NBUPPER    1
#define SMAG_BASSTAT_BASIC      2
#define SMAG_BASSTAT_SUPERBASIC 3
#define SMAG_RCINDIC_OK         0
#define SMAG_RCINDIC_NONOPT     1
#define SMAG_RCINDIC_INFEAS     2
#define SMAG_RCINDIC_UNBND      3

#define SMAG_LOGMASK    0x1
#define SMAG_LISTMASK   0x2
#define SMAG_STATUSMASK 0x4
#define SMAG_ALLMASK    SMAG_LOGMASK|SMAG_LISTMASK|SMAG_STATUSMASK

#define SMAG_INT_NA  -2
#define SMAG_DBL_NA  -1e20

#define SMAG_STATUS_OVERWRITE_NEVER    0
#define SMAG_STATUS_OVERWRITE_ALWAYS   1
#define SMAG_STATUS_OVERWRITE_IFDUMMY  2

#define SMAG_ERR_NO_MEMORY                 101
#define SMAG_ERR_HESSIAN_MEMORY_EXHAUSTED  102

/* SMAG_CALLCONV is the calling convention used for the SMAG routines.
 * leave empty for now, but __stdcall on Windows is an alternative */
#if ! defined(SMAG_CALLCONV)
# if ! defined(_WIN32)
#   define SMAG_CALLCONV
# else
#   define SMAG_CALLCONV
# endif
#endif

#if ! defined(CB_CALLCONV)
# if ! defined(_WIN32)
#   define CB_CALLCONV
# else
#   define CB_CALLCONV __stdcall
# endif
#endif

typedef struct smagRec     *smagHandle_t;
typedef struct smagDictRec *smagDictHandle_t; /* should this be separate?? */

typedef void
(SMAG_CALLCONV * smagMsgCB_t) (smagHandle_t prob, unsigned int mode,
			       const char *msg, int msgLen);
typedef struct smagHesDataRec {
  /* info for the symmetric, sparse Hessian */
  /* assumes we store the lower triangle in colPtr, rowIdx form  */
  int *colPtr;
  int *rowIdx;
  int lowTriNZ;
  int diagNZ;
  int NZ;				/* in entire Hessian:2*lowtri-diag */
} *smagHesDataHandle_t;

struct smagObjGradRec;
typedef struct smagObjGradRec {
  double dfdj;				/* dObj / d_j */
  struct smagObjGradRec *next;
  int j;				/* variable index, [0..n) */
  unsigned char degree;			/* of f as function of x_j */
                                        /*   0 -> f is not a func of x_j */
                                        /*   1 -> f is linear in x_j */
                                        /*   2 -> f is quadratic in x_j */
                                        /* 255 -> f's derivs never vanish */
} smagObjGradRec_t;

struct smagConGradRec;
typedef struct smagConGradRec {
  double dcdj;				/* dc_i / d_j */
  struct smagConGradRec *next;
  int j;				/* variable index, [0..n) */
  int jacOff;				/* offset for column-wise Jacobian,
					 * [0..nzCount) */
  unsigned char degree;			/* see smagObjGradRec above */
} smagConGradRec_t;

/* NL instructions, etc., in original GAMS form */
typedef struct gamsNLdata {
  unsigned int *instr;
  int          *startIdx;		/* of NL instr. for row i:
					 * Fortran-based, 0 == linear row */
  int          *numInstr;		/* number of instr. for row i:
					 * 0 == linear row */ /*  */
} gamsNLdata_t;

/* NL instructions, stripped form */
typedef struct strippedNLdata {
  unsigned int *instr;
  int          *startIdx;		/* of NL instr. for row i: */
					/* Fortran-based, 0 == linear row */
  int          *numInstr;		/* number of instr. for row i: */
					/* 0 == linear row */
  double       *s;			/* intermediate values */
  double       *sbar;			/* adjoints of intermediate values */
  double       *dfdx;			/* gradient values */
  double       *nlCons;			/* GAMS constants */

  int lenins;				/* combined length of NL instr. */
  int maxins;				/* maximum length of NL instr. */
  int nrows;
  int ncols;
} strippedNLdata_t;

typedef enum smagModelType {
  procNONE = 0,
  procLP,
  procMIP,
  procRMIP,
  procNLP,
  procMCP,
  procMPEC,
  procRMPEC,
  procCNS,
  procDNLP,
  procRMINLP,
  procMINLP,
  procQCP,
  procMIQCP,
  procRMIQCP,
  procDIM
} smagModelType_t;

/* values read directly from the control file
 * and left UNCHANGED are put here
 */
typedef struct gmsCFRec {
  double cheat;
  double core;				/* request core */
  double workFactor;			/* multiplier for memory estimates */
  double cutoff;
  double objVarScale;
  double optca, optcr;
  double reslim;
  double ropt[5];			/* real user options */
  double vlpinf;			/* + infinity */
  double vlminf;			/* - infinity */
  double vlundf;			/* undefined */
  double valna;				/* not available */
  double valeps;			/* epsilon */
  double valacr;			/* acronym */
  double tryInt;		 	/* start with integer feasible point */
  /* matrix data */
  int *geqnSense;			/* GAMS equation sense */
  int *growNB;				/* 1 if GAMS row is nonbasic */
  int *growNZ;				/* nonzero count for GAMS row */
  int *gcolMatch;			/* matching col for GAMS row */
  int *gcolNB;				/* 1 if GAMS col is nonbasic */
  int *gcolType;			/* GAMS column type (cont,int,bin,..)*/
  int *gcolSOS;                         /* Number of SOS col is in */
  int *gcolPtr;				/* GAMS Jacobian column pointers,
					 * 0-based,
					 * where the cols have no wasted space,
					 * so you get
					 * len(j) = ptr(j+1) - ptr(j) */
  int *gNLtype;				/* GAMS Jacobian markers: originally
					 * 0 if linear, 1 if not */
  int *growIdx;				/* GAMS row indices, 0-based */
  double *grhs;				/* GAMS RHS */
  double *gpi;				/* GAMS marginal: f.m */
  double *gfunc;			/* lhs - rhs; cons level */
  double *gfscale;			/* GAMS row scaling f.scale */
  double *gxlb;				/* GAMS lower bound x.lo */
  double *gxl;				/* GAMS x.l */
  double *gxub;				/* GAMS x.ub */
  double *gxrc;				/* GAMS reduced cost x.m */
  double *gxprior;                      /* GAMS priority x.prior */
  double *gxscale;                      /* GAMS col scaling x.scale */
  double *gJval;			/* GAMS Jacobian values, column-wise:
					 * (see gcolPtr, growIdx) */

  int controlFileVersion;
  int gamsVersion;			/* CMEX revision number, e.g. verXXX */
  int nRows;
  int cof2rd;			/* coeffs to read before reading next header */
  int daybas;			/* base date (Julian) */
  int daycheck;			/* date checking code */
  int daynow;			/* today's date (Julian) */
  int domlim;			/* domlim option */
  int dumpar;			/* if > 0, show results of file I/O */
  int filtyp;			/* filetype (0=coded, 1..=binary) */
  /* this next section deals with GAMS External(GE) functions */
  int ge_used;			/* external functions were used */
  int ge_nrows;			/* # of external functions */
  int ge_ncols;			/* # of vars in external functions */
  int ge_nnzs;			/* # of nonzeros (all NL) in ext. functions */
  int ge_nlibs;			/* currently always 1 */
  int gfdbug;			/* if > 0, print debugging output */
  int gpgwid, gpgsiz;		/* page width and size of status file */
  int iobvar;			/* objective variable */
  int icheat;
  int icutof;
  int idir;
  int ignbas;			/* ignore basis */
  int ilog;			/* use logfile or not */
  int iopt[5];			/* integer user options */
  int itnlim;
  int iscopt;			/* <model>.scaleopt */
  int keep;			/* gams=0, gamskeep=1 */
  int modin;			/* model indicator */
  int nlbls;			/* number of labels  (for dict file) */
  int nlclsmb;			/* number of local symbols  (for dict file) */
  int nindx;			/* total # index positions (for dict file) */
  int nrows;			/* number of equations */
  int nlrows;			/* number of nonlinear equations */
  int nosos1;			/* number of special ordered sets type 1 */
  int nosos2;			/* number of special ordered sets type 2 */
  int numrhs;			/* number of nonzero rhs elements */
  int ncols;			/* number of columns */
  int nlcols;			/* number of nonlinear columns */
  int nnz;			/* number of nonzero elements */
  int nlnz;			/* number of non-linear nonzero elements */
  int ncont;			/* number of continuous vars */
  int ndisc;			/* number of discrete vars */
  /* ncont + ndisc should be ncols */
  int nbin;			/* number of binary vars */
  int numint;			/* number of integer vars */
  int nsos1;			/* number of sos1 vars */
  int nsos2;			/* number of sos2 vars */
  int nsemi;			/* number of semi-continuous vars */
  int nsemii;			/* number of semi-cont integer vars */
  int nPositive;			/* number of positive vars */
  int nNegative;			/* number of negative vars */
  int nFree;				/* number of free vars */
  int mcpMatches;			/* rows paired with variables */
  int unmatchedRows;		/* rows without a matching var */
  int unmatchedFixed;		/* rows paired with a fixed var not in model */
  int nConeRows;
  int nfcols;			/* number of fixed vars (all types) */
  int nfcont;			/* number of fixed continuous vars */
  int nfbin;			/* number of fixed binary vars */
  int nfint;			/* number of fixed integer vars */
  int nfsos1;			/* number of fixed sos1 vars */
  int nfsos2;			/* number of fixed sos2 vars */
  int nfsemi;			/* number of fixed semi-continuous vars */
  int nfsemii;			/* number of fixed semi-cont integer vars */
  int maxcol;			/* max. number of nz. elements in a column */
  int maxInsLen;       /* max length non-linear code, single instr. */
  int minbin;			/* min. number of binary variables to      */
  int nodlim;
  int lennlc;			/* length non-linear code */
  int ncons;			/* number of constants in nl constant pool */
  int nscr;			/* highest scratch location */
  int instrStartIdx;		/* experimental: check with Steve
				 * offset for NL code arrays:
				 * 0 is CIOLIB default, assumed for C,
				 *   must -- instruction address field on read
				 * 1 used if Fortran or G2D interprets instr,
				 * and is the assumed by CMEX on writing instr */
  int ostype;			/* operating system identifier */
  int priots;
  int rowTypeCount[SMAG_NROWTYPES];	/* =e=, =g=, =l=, =n=, =x=, =c= */
  /* special statistics "for Partha", but used by everybody */
  int reform;				/* unused??? */
  int slpnzo;			/* nonzero entries in objective column */
  int slplro;			/* last row in objective column */
  int slpbno;			/* 0 if obj var is unbounded,
				 * probably 1 (definitely ~0) otherwise */

  int sysout;			/* echo status file to gams output */
  int useopt;			/* use option file */
  int SBBFlag;			/* 0 <==> not in SBB mode */
  int objEquType;		/* dupe of the matrix file */
  int objVarType;		/* dupe of the matrix file */
  int dictFileType;		/* ones digit: ASCIIDictType
				 *             (old "dictFileWritten" for ASCII dictionary)
				 * tens digit: GDXDictType
				 */
  int noCopyright;		/* to screen */
  int IDEflag;
  char logFileName[256];
  char statusFileName[256];
  char matrixFileName[256];
  char instrFileName[256];
  char optFileName[256];
  char solFileName[256];
  char dictFileName[256];
  char controlFileName[256];
  char SBBOptFileName[256];
  char SBBSaveFileName[256];
  char SBBRestartFileName[256];
  char SBBPCSaveFileName[256];
  char workingDir[256];
  char sysDir[256];
  char scrDir[256];
  char curDir[256];
  char lice1[80], lice2[80], lice3[80], lice4[80], lice5[80];
} gmsCFRec_t;

typedef struct SBBBoundRec {
  int    *colIdx;				/* in 1..n */
  double *lb;
  double *ub;
  int nBounds;
  int nextBound;
} SBBBoundRec_t;
typedef SBBBoundRec_t *SBBBoundHandle_t;

typedef struct SBBData {
  double rVec[2];
  double readAheadLB;
  double readAheadUB;

  FILE *fpSave;
  FILE *fpRestart;
  SBBBoundHandle_t boundHandle;
  int    *discIdx;
  double *discLB;
  double *discUB;

  int iVec[6];
  int discCounter;
  int readAheadIdx;
  int recLen;
  int xLevPos;
  int fMargPos;
  int varStatPos;
  int eqnStatPos;
  int nDisc;

  char restartFile[256];
  char saveFile[256];
} SBBData_t;

/* we aren't hiding this structure (all or nothing in C),
 * but we'd hide some if we could
 */
struct smagRec {
  /* not exposed to user */
  int dummy;
  int nativeIsBigEndian;
  int dataIsBigEndian;
  int swapEndian;		/* native endian-ness != data endian-ness */
  int objFlavor;		/* either OBJ_FUNCTION or OBJ_VARIABLE */
  int squeezeFixedVars;			/* ~0 or 0, true or false */
  int squeezeFixedRows;			/* ~0 or 0, true or false */
  int squeezeFreeRows;			/* ~0 or 0, true or false */
  int hessMergeStyle;
  /* hessMergeStyle is temporary:
   *  0=old style
   *  1=use g2dMergeHesStruct
   *  2=use g2dMergeHesStructCP
   */
  /* other options to consider:
   * reformulate into a system of equalities only?
   */

  /* niceObjRow = true if GAMS model has the "nice" objective var */
  /* ???  adds linear obj row: e_{objvar}
   * ???  else SMAG morphs obj row to remove obj var */
  int niceObjRow;
  int stdOutput;			/* open standard GAMS output files,
					 * i.e. log and listing/status */
  int ctChar;
  int statusCopyFlag;			/* on/off */
  int minim;				/* 1 for min, -1 for max, 0 for no obj */
  int minOnly;
  int logOption;
  int logFlush;
  int binaryIO;
  int bufSize, hfSize;
  int readDuals;
  int readRowNZCounts;
  int readScales;
  int readMatches;
  int rowCount;
  int colCount;
  int  nzCount;
  int rowCountNL;
  int colCountNL;
  int objColCount;
  int objColCountNL;
  int gObjRow;
  int gObjCol;
  double  inf;
  double gObjCoef;
  double gObjFactor;
  double nodUsd; 		/* tail record 1 */
  double numInfes;		/* tail record 2 */
  double numNOpt;		/* tail record 3 */
  				/* tail record 4 not used */
  double objEst;		/* tail record 5 */
  double resCalc;		/* tail record 6 */
  double resDeriv;		/* tail record 7 */
  double resIn;			/* tail record 8 */
  double resOut;		/* tail record 9 */
  double sumInfes;		/* tail record 10 */
  				/* tail record 11 not sure */
  double rObj;			/* tail record 12 */
  smagMsgCB_t msgCB;
  smagModelType_t modType;
  char modTypeName[8];
  char controlFileName[256];
  FILE *fpLog;
  FILE *fpStatus;
  gmsCFRec_t gms;
  SBBData_t SBBData;
  int    *rowMapG2S;		/* rowMapG2S[gi] = si, where
				 * gi is a GAMS index, si a SMAG index */
  int    *rowMapS2G;		/* rowMapS2G[si] = gi, where
				 * gi is a GAMS index, si a SMAG index */
  int    *colMapG2S;		/* colMapG2S[gj] = sj, where
				 * gj is a GAMS index, sj a SMAG index */
  int    *colMapS2G;		/* colMapInv[sj] = gj, where
				 * gj is a GAMS index, sj a SMAG index */
  double *gx;			/* level values in GAMS space */

  /* read-only */
  int    *colLen;
  int    *colType;
  int    *colSOS;
  unsigned char *colFlags;
  double *colLB;
  double *colLev;
  double *colUB;
  double *colRC;
  double *colPriority;
  double *colScale;
  int    *rowLen;
  int    *rowType;
  unsigned char *rowFlags;
  double *rowRHS;
  double *rowPi;
  double *rowLB;
  double *rowUB;
  double *rowScale;
  double objScale;
  struct smagObjGradRec *objGrad;
  struct smagConGradRec **conGrad;
  strippedNLdata_t snlData;
  smagHesDataHandle_t hesData;
};

#if defined(__cplusplus)
extern "C" {
#endif

  smagHandle_t
  smagInit (const char *controlFileName);
  int
  smagClose (smagHandle_t prob);
  int
  smagCloseLog (smagHandle_t prob);
  int
  smagColCount (smagHandle_t prob);
  int
  smagColCountNL (smagHandle_t prob);
  int
  smagEvalConFunc (smagHandle_t prob, const double x[], double conLevel[]);
  int
  smagEvalConiFunc (smagHandle_t prob, int i, const double x[],
		    double *lev, int *numErr);
  int
  smagEvalConGrad (smagHandle_t prob, const double x[]);
  int
  smagEvalConiGrad (smagHandle_t prob, int si, const double x[],
		    double *lev, int *numErr);
  int
  smagEvalLagHess (smagHandle_t prob,
		   const double x[], double objMult, const double u[],
		   double *W, int lenW, int *numErr);
  int
  smagEvalLagHessV (smagHandle_t prob,
		    const double x[], double objMult, const double u[],
		    const double *v, double *Wv, int *numErr);
  int
  smagEvalObjFunc (smagHandle_t prob, const double x[], double *objVal);
  int
  smagEvalObjGrad (smagHandle_t prob, const double x[], double *objVal);
  double
  smagGetCPUTime (smagHandle_t prob);
  double
  smagGetInf (smagHandle_t prob);
  int
  smagHessInit (smagHandle_t prob);
  int
  smagMinim (smagHandle_t prob);
  smagModelType_t
  smagModelType (smagHandle_t prob);
  char *
  smagModelTypeName (smagHandle_t prob);
  int
  smagNZCount (smagHandle_t prob);
  int
  smagObjColCount (smagHandle_t prob);
  int
  smagObjColCountNL (smagHandle_t prob);
  int
  smagOpenLog (smagHandle_t prob);
  int
  smagOpenLogC (smagHandle_t prob, FILE **fp, char *errMsg, int errMsgLen);
  int
  smagOpenStatusC (smagHandle_t prob, int appendFlag, FILE **fp,
		   char *errMsg, int errMsgLen);
  /* smagOutput is meant to be called only internally */
  int
  smagOutput (smagHandle_t prob, unsigned int mode, const char *fmt, ...);
  int
  smagPrint (smagHandle_t prob, unsigned int mode, const char *fmt, ...);
  void
  smagReadModel (smagHandle_t prob);
  int
  smagReadModelStats (smagHandle_t prob);
  int
  smagReportSolBrief (smagHandle_t prob, int modStat, int solStat);
  int
  smagReportSolStats (smagHandle_t prob, int modStat, int solStat,
		      int iterUsed, double resUsed, double objVal,
		      int domErrors);
  int
  smagReportSolX (smagHandle_t prob, int iterUsed, double resUsed,
		  int domErrors, const double rowMarg[], const double colLev[]);
  int
  smagReportSolFull (smagHandle_t prob, int modStat, int solStat,
		     int iterUsed, double resUsed, double objVal,
		     int domErrors,
		     const double rowLev[], const double rowMarg[],
		     const unsigned char rowBasStat[], const unsigned char rowIndic[],
		     const double colLev[], const double colMarg[],
		     const unsigned char colBasStat[], const unsigned char colIndic[]);
  int
  smagRowCount (smagHandle_t prob);
  int
  smagRowCountNL (smagHandle_t prob);
  int
  smagSetMessageCB (smagHandle_t prob, smagMsgCB_t msgCB);
  void
  smagSetObjEst (smagHandle_t prob, double objEst);
  void
  smagSetObjFlavor (smagHandle_t prob, int objFlavor);
  void
  smagSetInf (smagHandle_t prob, double inf);
  void
  smagSetPref (smagHandle_t prob);
  void
  smagSetSqueezeFreeRows (smagHandle_t prob, int squeezeFreeRows);
  void
  smagSetNodUsd (smagHandle_t prob, double nodUsd);
  void
  smagSetNumInfes (smagHandle_t prob, double numInfes);
  void
  smagSetNumNOpt (smagHandle_t prob, double numNOpt);
  void
  smagSetResCalc (smagHandle_t prob, double resCalc);
  void
  smagSetResDeriv (smagHandle_t prob, double resDeriv);
  void
  smagSetResIn (smagHandle_t prob, double resIn);
  void
  smagSetResOut (smagHandle_t prob, double resOut);
  void
  smagSetRObj (smagHandle_t prob, double rObj);
  void
  smagSetInfes (smagHandle_t prob, double sumInfes);
  int
  smagStdOutputStart (smagHandle_t prob, int statusOverwrite,
		      char *errMsg, int errMsgSiz);
  int
  smagStdOutputStop (smagHandle_t prob, char *errMsg, int errMsgSiz);
  void
  smagStdOutputPrintX (smagHandle_t prob, unsigned int mode, const char *msg,
		       int addNewline);
  void
  smagStdOutputPrint (smagHandle_t prob, unsigned int mode, const char *msg);
  void
  smagStdOutputPrintLn (smagHandle_t prob, unsigned int mode, const char *msg);
  void
  smagStdOutputFlush (smagHandle_t prob, unsigned int mode);

#if defined(__cplusplus)
}
#endif

#endif /* #if ! defined(_SMAG_H_) */
