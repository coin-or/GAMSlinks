/*
 * nliolib.h
 *
 *  Copyright (c) 1991 GAMS Development Corp.
 *                     1520 New Hampshire Ave. NW
 *		       Washington DC 20036
 */


#if ! defined(_NLIOLIB_H_)
#define       _NLIOLIB_H_

#if ! defined(JUST_NEWNL)
# define JUST_NEWNL
#endif

#if ! defined(JUST_NEWNL)
#define INPUSN	1	  /*  push endog */
#define INPUSC	2	  /*  push constant */
#define INSTOR	3	  /*  store row */
#define INPLUS	4	  /*  plus */
#define INADDV	5	  /*  add variable */
#define INADDI	6	  /*  add immediate */
#define INADDL	7	  /*  add local */
#define INMIN 	8	  /*  minus */
#define INSUBV	9	  /*  subtract variable */
#define INSUBI	10	  /*  subtract immediate */
#define INSUBL	11	  /*  subtract local */
#define INMUL	12	  /*  multiply */
#define INMULV 	13	  /*  multiply variable */
#define INMULI	14	  /*  multiply immediate */
#define INMULL	15	  /*  multiply local */
#define INDIV 	16	  /*  divide */
#define INDIVV	17 	  /*  divide variable */
#define INDIVI	18	  /*  divide immediate */
#define INDIVL	19	  /*  divide local */
#define INDVS	20	  /*  special divide (with special check) */
#define INDVSL	21	  /*  special divide local */
#define INIPW	22	  /*  integer power */
#define INIPWV 	23	  /*  integer power variable */
#define INIPWI	24	  /*  integer power immediate */
#define INRPW 	25	  /*  real power */
#define INRPWV	26	  /*  real power variable */
#define INRPWI	27	  /*  real power immediate */
#define INUMIN	28	  /*  unary minus */
#define INMINV 	29	  /*  unary minus variable */
#define INSQR	30	  /*  square */
#define INSQRV 	31	  /*  square var */
#define INSQRT 	32	  /*  square root */
#define INSQTV 	33	  /*  square root var */
#define INEXP 	34
#define INEXPV	35
#define INLOG	36
#define INLOGV 	37
#define INL10 	38	  /* 10 log */
#define INL10V	39	  /* 10 log var */
#define INCOS 	40	  /* cosine */
#define INCOSV 	41	  /* cosine var */
#define INSIN 	42	  /* sine */
#define INSINV 	43	  /* sine var */
#define INATN 	44	  /* arctan */
#define INATNV	45	  /* arctan var */
#define INDTN 	46	  /* derivative arctan */
#define INDTNV	47	  /* derivative arctan var */
#define INERF 	48	  /* error function */
#define INERFV	49	  /* error function var */
#define INDRF 	50	  /* derivative error function */
#define INDRFV	51	  /* derivative error function var */
#define INABS	52
#define INCCL	53	  /* conditional clear local */
#define INCUL 	54	  /* conditional umin local */
#define INTSTL 	55	  /* test lower than */
#define INCMPL	56	  /* compare lower than */
#define INCMPG	57	  /* compare greater than */
#define INSEL	58	  /* select */
#define INSWAP	59
#define INDROP 	60
#define INPUSL	61	  /* push local */
#define INPOPL	62	  /* pop local */
#define INKEP1	63	  /* keep 1 */
#define INKEP2 	64	  /* keep 2 */
#define INDERV	65	  /* deriv */
#define INHEAD	66	  /* header */
#define INPUML	67	  /* push umin local */
#define INSTSR	68	  /* store scaled row */
#define INSTSG	69	  /* store scaled gradient */
#define INEQUS	70	  /* equation scale */
#define INLNOB	71	  /* execute linear part of combined obj */
#define INENDC	72	  /* end of combined obj */
#define INSTRP	73	  /* store derivative of combined obj */
#define INEND 	74	  /* last instructon number */

#define INLAST	71	  /* last instruction delivered by GAMS */

/* extra instructions added by Arne */

#define INPUSP (INEND+1)   /* push combined objective coefficient */
#define INPUS0 (INPUSP+1)  /* push combined objective function value on stack */
#define ININIO (INPUS0+1)  /* Initialize combined objective coefficient to 0. */
#define MAXINS133	ININIO     /* largest instruction number */
#endif /* if ! defined(JUST_NEWNL) */

#define nlPushV      1         /* push variable */
#define nlPushI      2         /* push immediate (constant) */
#define nlStore      3         /* store row */
#define nlAdd        4         /* add */
#define nlAddV       5         /* add variable */
#define nlAddI       6         /* add immediate */
#define nlAddL       7         /* add local */
#define nlSub        8         /* subtract */
#define nlSubV       9         /* subtract variable */
#define nlSubI      10         /* subtract immediate */
#define nlSubL      11         /* subtract local */
#define nlMul       12         /* multiply */
#define nlMulV      13         /* multiply variable */
#define nlMulI      14         /* multiply immediate */
#define nlMulL      15         /* multiply local */
#define nlDiv       16         /* divide */
#define nlDivV      17         /* divide variable */
#define nlDivI      18         /* divide immediate */
#define nlDivL      19         /* divide local */
#define nlUMin      20         /* unary minus */
#define nlUMinV     21         /* unary minus variable */
#define nlSwap      22         /* swap two positions on stack top */
#define nlPushL     23         /* push local */
#define nlPopL      24         /* pop local */
#define nlPopDeriv  25         /* pop derivative */
#define nlHeader    26         /* header */
#define nlUMinL     27         /* push umin local */
#define nlStoreS    28         /* store scaled row */
#define nlPopDerivS 29         /* store scaled gradient */
#define nlEquScale  30         /* equation scale */
#define nlEnd       31         /* end of instruction list */
#define nlCallArg1  32
#define nlCallArg2  33
#define nlCallArgN  34
#define nlFuncArgN  35
#define nlPushS     36
#define nlPopup     37
#define nlArg       38
#define nlMulIAdd   39
#define nlPushZero  40
#define nlMulPop1   41
#define nlMulPop2   42
#define nlMulPop    43
#define nlAddPop    44
#define nlSubPop    45
#define nlGetConst  46
#define nlMulConst1 47
#define nlMulConst2 48
#define nlMulConst  49
#define nlNegLocal  50
#define nlGetLocal  51
#define nlSetLocal1 52
#define nlSetLocal2 53
#define nlSetLocal  54
#define nlGetGrad   55
#define nlPushIGrad 56

#define nlChk       57
#define nlAddO      58
#define nlPushO     59
#define nlInvoc     60
#define nlStackIn   61

#define MAXINS      61		      /* largest instruction number */


				/* type instructions as unsigned! */
#if 0
# define CIO_MAXADD  16777215	/* max. 24-bit address: 2**24 - 1 */
#else
# define CIO_MAXADD  0x3ffffff	/* max. 26-bit address: 2**26 - 1 */
# define CIO_OPCODE_BITS    26
#endif

/* update GETOPCODE, GETADDR, CIO_PACKINSTR
 * if instruction packing scheme changes
 */
#define GETOPCODE(instr,i) ((instr)[(i)] >> CIO_OPCODE_BITS)
#define GETADDR(instr,i)   ((instr)[(i)] & 0x3ffffff)
#define CIO_PACKINSTR(opCode,addr) (((opCode) << CIO_OPCODE_BITS) | (addr))

#define IS_STOREDERIV(i) (nlPopDeriv  == (i) || \
                          nlPopDerivS == (i))
#define IS_STOREFUNC(i)  (nlStore  == (i) || \
                          nlStoreS == (i))

#define CIO_NLSTACK 100


extern int nvrlc;

#if defined(__cplusplus)
extern "C" {
#endif

void
gciGetInstrStarts (unsigned int *instr,
		   int startIdx[],	/* ptrBase-based */
		   const int ptrBase,
		   int numInstr[]);	/* count of instr, if non-NULL */
void gfsteq (unsigned int *instr, int strteq[]);
void gfnsto (unsigned int *instr, int iseq, int opCode, int addr);
void gfntrp (double *x, double *g, double *delg,
	     unsigned int *instr, double *cons, int *usrnumerr);
void gfnint (unsigned int *instr, int istart, int *inext, int *irowseq);
void gfrbin (int *ins1, int *ins2, FILE *f);
void gfwbin (int iopcod, int iaddr, FILE *f);
void gfwcnt (char *, iolib_t *);
void gfwrmat(char *, double *, int *, int *, int *,
	     double *, int *, double *,
	     double *, double *, double *, int *,
	     int *, int *, double *, int *,
	     iolib_t *, int, int);
void gfrins (unsigned int *, double *);
void gfdmpa (FILE *fp, unsigned int *instr);
void gfdmpe (FILE *fp, unsigned int *instr);
void gfjtyp (unsigned int *instr, int *irow, int *jcol, int *jtyp);
void gfjtypX (unsigned int *instr, int *irow, int *jcol, int ncols,
	      int *jtyp, int nnz);
void gfpmjj (unsigned int *instr, int *irow, int *jtyp);
void gfpmjjX (unsigned int *instr, int *irow, int *jtyp, int nnz);
void gfprmj (unsigned int *instr, int jtyp, int knew);
void gfwmrow (double, double, int, int, FILE*);
void gfwcof  (double,int,int,FILE*);
void gfwmcol (int,
	      int,
	      double,
	      double,
	      double,
	      int,
	      int,
	      BOOLE,
	      double,
	      FILE*);
int
cioSetErrorMethod (int i);
int
cioGetErrorMethod (void);
int
cioSetErrorMessagePrint (int i);
int
cioGetErrorMessagePrint (void);
int
cioSetTestHeader (int i);
int
cioGetTestHeader (void);

/*****************************************************************************/
/* Functions added by TSM to perform permutations of the rows/cols and       */
/* jacobian elements.                                                        */
/*****************************************************************************/

void cioPermuteRows (unsigned int *instr, int *rperm);
void cioPermuteCols (unsigned int *instr, int *cperm);
void cioPermuteJac  (unsigned int *instr, int *irow, int *jtyp,
                     int *iwork, int nnz);
void cioPermuteJacL (unsigned int *instr, int *irow, int *jtyp,
                     int *jperm, int *iwork, int nnz);

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_NLIOLIB_H_) */
