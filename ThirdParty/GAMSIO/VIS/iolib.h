/* definitions:
 *
 * define one of the following in the makefile, using -D....
 *
 *   AIX    (RS/6000, cc compiler)
 *   SUN    (SUN SPARC, SUNOS, cc k&r compiler)
 *   HP7    (HP9000/700 series, did not try out 800 for a long time)
 *   AXU    (AXP OSF)
 *   etc.   ....
 * MSDOS:  if defined it is a system with MSDOS file system
 * UNIX :  if defined it is a system with UNIX file system
 * CIO_INT_IS_4 : ints are 4 bytes
 * CIO_INT_IS_2 : ints are 2 bytes
 * NOSTRDUP : standard C lib has no 'strdup'
 * NOSTRICMP : standard C lib has no 'stricmp'
 * NOSTRNICMP : standard C lib has no 'strnicmp'
 *
 *
 * Change Log:
 *   oct 99, EK: removed ANSI define (assume all C compilers are ANSI compatible)
 */

#if ! defined(_IOLIB_H_)
#define       _IOLIB_H_

#include "bbinfo.h"
#include "gclproc.h"

#define CB_CALLCONV

#if defined(LEG) || defined(LEI)
#define CIO_INT_IS_4
/* #define NOSTRDUP */
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(LNX) || defined(LX3) || defined(LXI) || defined(LXY) || defined(SIG)
#define CIO_INT_IS_4
/* #define NOSTRDUP */
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(DAR) || defined(DII)
#define CIO_INT_IS_4
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(LI4)
#define CIO_INT_IS_4
/* #define NOSTRDUP */
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(LP4)
#define CIO_INT_IS_4
/* #define NOSTRDUP */
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(LZ4)
#define CIO_INT_IS_4
/* #define NOSTRDUP */
#define NOSTRICMP
#define NOSTRNICMP
#if ! defined(UNIX)
# define UNIX
#endif
#endif

#if defined(L95) || defined(VIS)
#if ! defined(MSDOS)
# define MSDOS
#endif
#define CIO_INT_IS_4
#undef  CB_CALLCONV
#define CB_CALLCONV __stdcall
#endif

#if defined(ITL) || defined(WEI)
#if ! defined(MSDOS)
# define MSDOS
#endif
#define CIO_INT_IS_4
#undef  CB_CALLCONV
#define CB_CALLCONV __stdcall
#endif

#if defined(WAT) || defined(WPL)
#if ! defined(MSDOS)
# define MSDOS
#endif
#define CIO_INT_IS_4
#undef  CB_CALLCONV
#define CB_CALLCONV __stdcall
#endif

#if defined(MGW)
#if ! defined(MSDOS)
# define MSDOS
#endif
#define CIO_INT_IS_4
#define NOSTRDUP
#define NOSTRICMP
#endif

#ifdef S86
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRICMP
#define NOSTRNICMP
#endif

#if defined(SOL) || defined(SOX)
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRICMP
#define NOSTRNICMP
#endif

#ifdef HP7
#define HP9000
#define HP800 /* for license info */
#endif

#ifdef HP7
#define setlinebuf(f) setvbuf(f, (char *)NULL, _IOLBF, 0)
#endif /* setlinebuf */

#ifdef HP9000
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRNICMP   /* use our own strnicmp */
#define NOSTRICMP   /* use our own stricmp */
#endif

#ifdef AIX
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRNICMP
#define NOSTRICMP
#endif

#ifdef AXU
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRNICMP
#define NOSTRICMP
#define HZ 60   /* they don't define this */
#endif

#ifdef AXV
#define OSVMS
#define CIO_INT_IS_4
#define NOSTRNICMP
#define NOSTRICMP
#endif

#if defined(SG3) || defined(SG4) || defined(SG6) || defined(SG8)
# define SGI
#endif

#ifdef SGI
#if ! defined(UNIX)
# define UNIX
#endif
#define CIO_INT_IS_4
#define NOSTRNICMP
#define NOSTRICMP
#endif


#define IOLIB_BUFSIZE 800      /* size of buffer: 800 bytes */

/* characteristics of stream I/O, file code 1 in gamscomp.txt */
#define STARTBUF1  0
#define ENDBUF1    799

/* characteristics of formatted I/O, file code 2 in gamscomp.txt */
#define STARTBUF2  4         /* bytes 1 to 4 are used for BOR info */
#define ENDBUF2    795       /* bytes 797 to 800 are used for EOR info */
#define BINBUFFRECLEN2 792    /* net record length with 4 byte header/footer:
                                800 - 4 - 4 */

#define GFHUGE    0.9999999999e300
#define GFDWARF   1.0E-20

#define LOVER     30		/* allowed versions of the control file */
#define HIVER     50

#define MAXFLN    256		/* includes null byte */

/*
 * #define MODNONE   0
 * #define LP        1
 */
#define MIP       PROC_MIP  /* used by DICOPT */
/*
 * #define RMIP      3
 */
#define NLP       PROC_NLP /* used by DICOPT */

#define MAXMODELTYP 20
#define MAXSOLVERS 100 /* who has 100 different solvers */

#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif


#define FREAD   0
#define FWRITE  1
#define FAPPEND 2

#define FRMBIN  0
#define FRMCOD  1

#define BOOLE int

#define EQ	0			/* =e= */
#define GE	1			/* =g= */
#define LE	2			/* =l= */
#define FR      3			/* =n= */
#define XE      4			/* =x= */
#define CE      5			/* =c= */
#define CIO_NROWTYPES (CE+1)
#define DE      6			/* =d=, just put in for future use */

/* demo size limits */
/* these should go away: the license library handles this stuff,
 * but I suppose somebody could be depending on these */
#define CIO_MAXN     300
#define CIO_MAXM     300
#define CIO_MAXNZ   2000
#define CIO_MAXNLNZ 1000
#define CIO_MAXDISC   50

/* should get these from global or similar */
#define CIO_PROCNAMELEN 11
#define CIO_ALGNAMELEN  11

typedef char tbuf[IOLIB_BUFSIZE];

typedef struct {
  int  bp;
  tbuf buf;
} tbufrec;


#define MAXLINE 512

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

typedef struct {
  /* structure order:
   * doubles:     8 bytes
   * pointers:    4 or 8 bytes
   * integers:    4 bytes
   * tbufrec :    byte aligned
   * characters:  byte aligned
   */

  double
    cheat,
    core,			/* request core */
    workFactor,			/* multiplier for memory estimates */
    cutoff,
    objVarScale,
    optca,
    optcr,
    reslim,
    robcof,
    ropt[5],			/* real user options */
    tryInt,			/* start with integer feasible point */
    usrpinf,			/* value for + infinity in user code */
    usrminf,			/* value for - infinity in user code */
    vlpinf,			/* + infinity */
    vlminf,			/* - infinity */
    vlundf,			/* undefined */
    valna,			/* not available */
    valeps,			/* epsilon */
    valacr;			/* acronym */

  SBBData_t *sbb;
  const char *gfctrl;		/* filename of control file */
  char
    *flndat,			/* filename data file */
    *flnins,			/* filename instruction file */
    *flnopt,			/* filename option file */
    *flnsta,			/* filename status file */
    *flnsol,			/* filename solution file */
    *flndic,			/* filename dictionary file */
    *flnlog,			/* filename logfile */
    *flnscr,			/* filename scratch file (<modname>.scr) */
    *flnsbbopt,			/* filename SBB option file */
    *flnsbbsav,			/* filename SBB bounds save file */
    *flnsbbres,			/* filename SBB bounds restart file */
    *flnsbbpcsav,		/* filename SBB pseudo-cost save file */
    *flncontrol,		/* filename control file */
    *gtime,			/* gams time */
    *gdate,			/* gams date */
    *gwrkdr,			/* gams working directory */
    *gsysdr,			/* gams system dir */
    *gscrdr,			/* gams scratch dir */
    *gcurdr,			/* gams current dir */
    *flnpar,
    *ge_libname,
    *flnmiptrace,
    *modeltype[MAXMODELTYP],	/* for instance LP, MIP, NLP ..... */
    *line1[MAXSOLVERS],
    *line2[MAXSOLVERS],
    *line3[MAXSOLVERS];

  /* was too complicated in conjunction with DICOPT. EK June 1999.
  FILE
    *gflibfile;			 for use inside library  */

  BOOLE
    gfbinf,			/* whether or not to use binary files */
    gfstok,			/* ok to write to status file */
    gfcnok,			/* ok to write to console file */
    ordercheck,
    noCopyright,		/* to screen */
    readDuals,			/* read actual duals for nonbasic cols,
				 * binding rows */
    readRowNZCounts,		/* read NZ counts for rows */
    readScales,			/* read scale factors for rows, cols */
    readMatches;		/* read matching column for each row */


  int
    age,			/* needed to calc time since expired */
    cof2rd,			/* coeffs to read before reading next header */
    daybas,			/* base date (Julian) */
    daycheck,			/* date checking code */
    daynow,			/* today's date (Julian) */
#if !defined(NEWDICT)
    dictFileWritten,		/* if nonzero, dictionary file written */
#endif
    domlim,			/* domlim option */
    dumpar,			/* if > 0, show results of file I/O */
    filtyp,			/* filetype (0=coded, 1..=binary) */
    /* this next section deals with GAMS External(GE) functions */
    ge_used,			/* external functions were used */
    ge_nrows,			/* # of external functions */
    ge_ncols,			/* # of vars in external functions */
    ge_nnzs,			/* # of nonzeros (all NL) in ext. functions */
    ge_nlibs,			/* currently always 1 */
    gfdbug,			/* if > 0, print debugging output */
    gfstts,
    /*  gfstts (gf-status) keeps track of the status of control flow */
    /*  through the library.                                         */
    gfmsta,
    gfnsta,
    gpgwid, gpgsiz,		/* page width and size of status file */
    IDEflag,
    iobvar,			/* objective variable */
    /* iobrow,		*/	/* objective row */
    /* this was never set! (since ver001).  Please check that
     * objective col has one entry (iolib.nzo) and then use iolib.slplro */
    icheat,
    icutof,
    idir,
    ignbas,			/* ignore basis */
    ilog,			/* use logfile or not */
    iopt[5],			/* integer user options */
    itnlim,
    iscopt,			/* <model>.scaleopt */
    keep,			/* gams=0, gamskeep=1 */

    modin,			/* model indicator */
    nlbls,			/* number of labels  (for dict file) */
    nlclsmb,			/* number of local symbols  (for dict file) */
    nindx,			/* total # index positions (for dict file) */

    nomodeltypes,		/* number of different model types */
    nosolvers,			/* number of solvers */
    nrows,			/* number of equations */
    nlrows,			/* number of nonlinear equations */
    nosos1,			/* number of special ordered sets type 1 */
    nosos2,			/* number of special ordered sets type 2 */
    numrhs,			/* number of nonzero rhs elements */
    numSolves,			/* number of solves */

    ncols,			/* number of columns */
    nlcols,			/* number of nonlinear columns */

    nnz,			/* number of nonzero elements */
    nlnz,			/* number of non-linear nonzero elements */

    ncont,			/* number of continuous vars */
    ndisc,			/* number of discrete vars */
    /* ncont + ndisc should be ncols */
    nbin,			/* number of binary vars */
    numint,			/* number of integer vars */
    nsos1,			/* number of sos1 vars */
    nsos2,			/* number of sos2 vars */
    nsemi,			/* number of semi-continuous vars */
    nsemii,			/* number of semi-cont integer vars */

    nPositive,				/* number of positive vars */
    nNegative,				/* number of negative vars */
    nFree,				/* number of free vars */

    mcpMatches,			/* rows paired with variables */
    unmatchedRows,		/* rows without a matching var */
    unmatchedFixed,		/* rows paired with a fixed var not in model */
    nConeRows,

    nfcols,			/* number of fixed vars (all types) */
    nfcont,			/* number of fixed continuous vars */
    nfbin,			/* number of fixed binary vars */
    nfint,			/* number of fixed integer vars */
    nfsos1,			/* number of fixed sos1 vars */
    nfsos2,			/* number of fixed sos2 vars */
    nfsemi,			/* number of fixed semi-continuous vars */
    nfsemii,			/* number of fixed semi-cont integer vars */

    maxcol,			/* max. number of nz. elements in a column */
    maxInsLen,			/* max length non-linear code, single instr. */
    minbin,			/* min. number of binary variables to      */
    nodlim,

    lennlc,			/* length non-linear code */
    ncons,			/* number of constants in nl constant pool */
    nscr,			/* highest scratch location */
    instrStartIdx,		/* experimental: check with Steve
				 * offset for NL code arrays:
				 * 0 is CIOLIB default, assumed for C,
				 *   must -- instruction address field on read
				 * 1 used if Fortran or G2D interprets instr,
				 * and is the assumed by CMEX on writing instr */
    ostype,			/* operating system identifier */
    priots,
    rowTypeCount[CIO_NROWTYPES],	/* =e=, =g=, =l=, =n=, =x=, =c=, =d= */
    reform;			/* reformulation */

  /* special statistics "for Partha", but used by everybody */
  int slpnzo;			/* nonzero entries in objective column */
  int slplro;			/* last row in objective column */
  int slpbno;			/* 0 if obj var is unbounded,
				 * probably 1 (definitely ~0) otherwise */

  int sysout;			/* echo status file to gams output */
  int useopt;			/* use option file */
  int gamsVersion;		/* GAMS build number */
  int controlFileVersion;	/* version number of the control file */
  int algdefault[MAXMODELTYP];	/* for instance MINOS5, BDMLP */
  int algchosen[MAXMODELTYP];	/*     id.                    */
  int algUsed;				/* when control file was generated */

  int SBBFlag;			/* 0 <==> not in SBB mode */
  int printEvalErrMsg;		/* ~0 <--> print NL eval msg to status file */
  int nEvalErrMsg;		/* count of NL eval msgs printed */
  int
    startbuf, endbuf,
    lenins,
    maxadd,
    rowcounter, colcounter, nzread;

  int dictFileType;			/* ones digit: ASCIIDictType
					 *             (old "dictFileWritten" for ASCII dictionary)
					 * tens digit: GDXDictType
					 */
  int ASCIIDictType;
  int GDXDictType;
  int				/* extra dictionary counts */
				/* from dupe of line 1 of dict file */
    nGSyms,
    nLSyms,
    nIndexSets,
    nUels,
    nIndexEntries,
    nDRows,
    nDCols,
    hasApos,
    dictFileOpt,
    nAcronyms,
    nModListSyms,
    dictVersion,
    lrgdim,
    lrgindx,
    lrgblk,
    ttlblk,
    nUelChars,
    nSymNameChars,
    nSymTextChars,
    nSymIndices,
    nSetTextChars,
    nRowUels,
    nColUels,
    objEquType,			/* dupe of the matrix file */
    objVarType;			/* dupe of the matrix file */


  tbufrec buffer;
  char
    glice1[MAXLINE],
    glice2[MAXLINE],
    glice3[MAXLINE],
    glice4[MAXLINE],
    glice5[MAXLINE];

} iolib_t;

typedef iolib_t tiolib;

/*  function GFRCNT */
typedef struct {
  double  xgv[30+1];		/* general */
  int     kgv[35+1];		/* general */
  BOOLE   lgv[30+1];		/* general */
  int     knv[30+1];		/* nlp */
  int     kmv[30+1];		/* mip */
} cntrec;

/* function GFRROW */
typedef struct {
  double  rdata[4+1];
  int     idata[3+1];
} rowrec;

typedef struct rowRec_t {
  double  rdata[6+1];		/* leave some slack */
  int     idata[6+1];		/* ditto */
} rowRec_t;

/* function GFRCOL */
typedef struct {
  double  cdata[4+1];
  int     idata[4+1];
} colrec;

typedef struct colRec_t {
  double  cdata[6+1];		/* leave some slack */
  int     idata[6+1];		/* ditto */
} colRec_t;

/* function GFMACH, note that all the sizes (IMACH) are missing:
   use sizeof() for this */
typedef struct {
  double  rmach[6+1];
} machrec;

#if ! defined(CONST)
#  define CONST const
#endif

/* use malloc from stdlib */
#define CIOMALLOC(PTR,TYPE,N) \
 ((PTR) = (TYPE *)malloc((N)*sizeof(TYPE)))
#define CIOFREE(X) do { \
  if (NULL != (X)) { free(X);(X) = NULL;} \
 } while (0)

#if defined(__cplusplus)
extern "C" {
#endif

extern iolib_t iolib;
extern FILE    *gfiosta;	/* status file */
extern FILE    *gfiolog;	/* logfile */

void CB_CALLCONV
gci2DData (int *ivec, int *ivecLen);
int
gciMIPTraceClose (void);
int
gciMIPTraceLine (char seriesID, int node, int giveint,
		 double seconds, double bestint, double bestbnd);
int
gciMIPTraceOpen (const char *fname, const char *solverID);
int
gciSign (CONST char *signature);
void
gciSetBufType (iolib_t *ioRec, int binType);
void
gciGetSrcInfo (char *srcName, int snLen,
	       int *verID,
	       char *srcDate, int sdLen);
void
gciGetINF (double *mInf, double *pInf);
void
gciReSetINF (void);
void
gciSetINF (double mInf, double pInf);

char *cGetAuditLine (CONST char *shortText, CONST char *dateString,
		     CONST char *platformCode, CONST char *solvID,
		     int major, int minor, int linkVer, int iolibVer,
		     int cfVer, CONST char *buildCode, CONST char *longText,
		     char *dest, int destLen);
char *ciostrdup (CONST char *s); /* private, don't use outside IOLIB */
void gferr (int msgid, char *sub);
void gfopnf(CONST char*, FILE**, int, int);
double gfclck(void);
void gfmach(machrec*);
void gfinit(void);
void gfsetft(iolib_t *ioRec, int newFt);
 int gfrcnt(BOOLE, BOOLE, cntrec *, const char *);
void gfWriteCntr (char *fln, iolib_t *ioRec, int minVer);
void gfstat(char**);
void gfopst(void);
void gfappst(void);
void gfappstx(void);
void gfstct(char*);
void gfsysid(char *name, int *sysid);
void gfrwer(int, int, char*);
void gfcler(int, int, char*);
void gfrcer(int, int, int, char*);
void gfpgsz(int*, int*);
void gfdat(char**);
void gfrrow(rowrec*);
void gfrrow2(rowrec *rowdata);
void gfrcol(colrec*);
void gfrcof(double*,int*,int*);
int
gfrsol (char *fln, int *modstat, int *solstat, int *itsusd,
	double *resusd, double *obj, int *domerr,
	double *eqlevel, double *dual, int *eqstat,
	int *eqbstat, const int *eqsign, const double *rhs,
	double *x, double *rc, int *colstat,
	int *colbstat, const double *lb, const double *ub,
	int m, int n);
void gfwsolflag (int solWritten);
void gfwsta(int, int, int, double, double, int);
void gfshea(int key, double val, FILE * f);
void gfwbnd(double);
void gfwnod(int);
void gfwrti(double);
void gfwcti(double);
void gfwwti(double);
void gfwdti(double);
void gfopti(char*);
void gftime (char **gmstim);
void gfdate (char **gmsdat);
void gfperr(int n, const char *smess, const char *errf, BOOLE tolog);
void gfwcol(double, double, int, int);
void gfwrow(double, double, int, int);
void gfwspvl (double, double, double, double,
	     double, double, FILE*);
void gfclos(void);
char *gflice(int);
iolib_t *iolib_address(void);
#ifdef NOSTRICMP
int stricmp(const char *s1, const char *s2);
#endif
#ifdef NOSTRNICMP
int strnicmp(const char *s1, const char *s2, int n);
#endif
#ifdef NOSTRDUP
char *strdup(const char *);
#endif
void gflsh(FILE *f);
void gfcsol(void);

void rdint1(FILE*, int*);
void wrint1(FILE*, int);
void rdint2(FILE*, int*);
void wrint2(FILE*, int);
void rdint4(FILE*, int*);
void wrint4(FILE*, int);
void rdreal(FILE*, double*);
void wrreal(FILE*, double);

void cioReadRow (rowRec_t *data);
void cioReadCol (colRec_t *data);
int
cioReadBestBound (double *bestBound);
int
cioReadNodeCount (int    *nodeCount);
int
cioReadReadTime  (double *readTime);
int
cioReadCalcTime  (double *calcTime);
int
cioReadDerivTime (double *derivTime);
int
cioReadWriteTime (double *writeTime);
void
cioSetNoAbort (int yesno);
void
cioSetCONEOK (int yesno);
void
cioSetDLLOK (int yesno);
void
cioSetSBBOK (int yesno);
int
cioSolverCap (const char *solver_line1, int modelType);
char *
cioStripDQuote (char *to, char *from, int toLen);

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_IOLIB_H_) */
