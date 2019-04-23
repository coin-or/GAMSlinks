#if ! defined(_G2DUTILS_H_)
#     define  _G2DUTILS_H_

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

#define G2D_MAXINS  61		      /* largest instruction number */

typedef enum g2dErr {
  errNone                 =  0,
  errUnknown              =  1,
  errInstrAddressNonzero  =  2,
  errInstrAddressTooLarge =  3,
  errInstrAddressNegative =  4,
  errInstrUnImplemented   =  5,
  errInstrCorrupt         =  6,
  errInstrInvalid         =  7,
  errCallArgSysErr        =  8,
  errNoJacRecords         = 10,
  errBadJacList           = 11,
  errNoHesRecords         = 15,
  errBadHesList           = 16,
  errStackNotEmpty        = 20,
  errBadCall              = 30,
  errAllocExhaust         = 40
} g2dErr_t;

#if ! defined(GETOPCODE)
# define GETOPCODE(instr,i) ((instr)[(i)] >> 24)
#endif
#if ! defined(GETADDRESS)
#define GETADDRESS(instr,i) ((instr)[(i)] & 0xffffff)
#endif
#if ! defined(IS_STOREDERIV)
#define IS_STOREDERIV(i) (nlPopDeriv  == (i) || \
                          nlPopDerivS == (i))
#endif
#if ! defined(IS_STOREFUNC)
#define IS_STOREFUNC(i)  (nlStore  == (i) || \
                          nlStoreS == (i))
#endif

void
g2dCErr (g2dErr_t err, const char *funcName);

#endif /* if ! defined(_G2DUTILS_H_) */
