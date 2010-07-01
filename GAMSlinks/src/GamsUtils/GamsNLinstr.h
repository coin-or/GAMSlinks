/* Copyright (C) 2008 GAMS Development and others
   All Rights Reserved.
   This code is published under the Common Public License.

   $Id$

   Author: Stefan Vigerske

   WARNING:
   The NL instruction (codes, format, semantics, etc.) are subject to change.
   Thus, there is no warranty that a code based on this file is going to work fine
   after changes in the GAMS I/O libraries as provided by GAMS.
   GAMS is NOT obligated to issue a warning when it changes the NL instructions.
*/

#ifndef GAMSNLINSTR_H_
#define GAMSNLINSTR_H_

#include "GAMSlinksConfig.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if GMOAPIVERSION >= 8
/** The opcodes of GAMS nonlinear expressions.
 */
typedef enum GamsOpCode_ {
	nlNoOp     =  0, /* no operation */
	nlPushV    =  1, /* push variable */
	nlPushI    =  2, /* push immediate (constant) */
	nlStore    =  3, /* store row */
	nlAdd      =  4, /* add */
	nlAddV     =  5, /* add variable */
	nlAddI     =  6, /* add immediate */
	nlSub      =  7, /* minus */
	nlSubV     =  8, /* subtract variable */
	nlSubI     =  9, /* subtract immediate */
	nlMul      = 10, /* multiply */
	nlMulV     = 11, /* multiply variable */
	nlMulI     = 12, /* multiply immediate */
	nlDiv      = 13, /* divide */
	nlDivV     = 14, /* divide variable */
	nlDivI     = 15, /* divide immediate */
	nlUMin     = 16, /* unary minus */
	nlUMinV    = 17, /* unary minus variable */
	nlSwap     = 18, /* swap two positions on stack top */
	nlHeader   = 19, /* header */
	nlStoreS   = 20, /* store scaled row */
	nlEquScale = 21, /* equation scale */
	nlEnd      = 22, /* end of instruction list */
	nlCallArg1 = 23,
	nlCallArg2 = 24,
	nlCallArgN = 25,
	nlFuncArgN = 26,
	nlPushS    = 27,
	nlPopup    = 28,
	nlArg      = 29,
	nlMulIAdd  = 30,
	nlPushZero = 31,
	nlChk      = 32, 
	nlAddO     = 33, 
	nlPushO    = 34,
	nlInvoc    = 35, 
	nlStackIn  = 36,
	MAXINS     = 37
} GamsOpCode;
#else
/** The opcodes of GAMS nonlinear expressions.
 */
typedef enum GamsOpCode_ {
   nlNoOp     =  0, /* no operation */
   nlPushV    =  1, /* push variable */
   nlPushI    =  2, /* push immediate (constant) */
   nlStore    =  3, /* store row */
   nlAdd      =  4, /* add */
   nlAddV     =  5, /* add variable */
   nlAddI     =  6, /* add immediate */
   nlAddL     =  7, /* add local */
   nlSub      =  8, /* minus */
   nlSubV     =  9, /* subtract variable */
   nlSubI     = 10, /* subtract immediate */
   nlSubL     = 11, /* subtract local */
   nlMul      = 12, /* multiply */
   nlMulV     = 13, /* multiply variable */
   nlMulI     = 14, /* multiply immediate */
   nlMulL     = 15, /* multiply local */
   nlDiv      = 16, /* divide */
   nlDivV     = 17, /* divide variable */
   nlDivI     = 18, /* divide immediate */
   nlDivL     = 19, /* divide local */
   nlUMin     = 20, /* unary minus */
   nlUMinV    = 21, /* unary minus variable */
   nlSwap     = 22, /* swap two positions on stack top */
   nlPushL    = 23, /* push local */
   nlPopL     = 24, /* pop local */
   nlPopDeriv = 25, /* pop derivative */
   nlHeader   = 26, /* header */
   nlUMinL    = 27, /* push umin local */
   nlStoreS   = 28, /* store scaled row */
   nlPopDerivS= 29, /* store scaled gradient */
   nlEquScale = 30, /* equation scale */
   nlEnd      = 31, /* end of instruction list */
   nlCallArg1 = 32,
   nlCallArg2 = 33,
   nlCallArgN = 34,
   nlFuncArgN = 35,
   nlPushS    = 36,
   nlPopup    = 37,
   nlArg      = 38,
   nlMulIAdd  = 39,
   nlPushZero = 40,
   nlMulPop1  = 41,
   nlMulPop2  = 42,
   nlMulPop   = 43,
   nlAddPop   = 44, 
   nlSubPop   = 45, 
   nlGetConst = 46, 
   nlMulConst1= 47, 
   nlMulConst2= 48, 
   nlMulConst = 49, 
   nlNegLocal = 50, 
   nlGetLocal = 51, 
   nlSetLocal1= 52, 
   nlSetLocal2= 53, 
   nlSetLocal = 54, 
   nlGetGrad  = 55, 
   nlPushIGrad= 56, 
   nlChk      = 57, 
   nlAddO     = 58, 
   nlPushO    = 59,
   nlInvoc    = 60, 
   nlStackIn  = 61,
   MAXINS     = 62
} GamsOpCode;
#endif

/** Names of GAMS nonlinear expressions for printing.
 */
extern const char* GamsOpCodeName[MAXINS];

/** The codes of GAMS nonlinear functions.
 */
typedef enum GamsFuncCode_ {fnmapval=0,fnceil,fnfloor,fnround,
    fnmod,fntrunc,fnsign,fnmin,
    fnmax,fnsqr,fnexp,fnlog,
    fnlog10,fnsqrt,fnabs,fncos,
    fnsin,fnarctan,fnerrf,fndunfm,
    fndnorm,fnpower,fnjdate,fnjtime,
    fnjstart,fnjnow,fnerror,fngyear,
    fngmonth,fngday,fngdow,fngleap,
    fnghour,fngminute,fngsecond,
    fncurseed,fntimest,fntimeco,
    fntimeex,fntimecl,fnfrac,fnerrorl,
    fnheaps,fnfact,fnunfmi,fnpi,
    fnncpf,fnncpcm,fnentropy,fnsigmoid,
    fnlog2,fnboolnot,fnbooland,
    fnboolor,fnboolxor,fnboolimp,
    fnbooleqv,fnrelopeq,fnrelopgt,
    fnrelopge,fnreloplt,fnrelople,
    fnrelopne,fnifthen,fnrpower,
    fnedist,fndiv,fndiv0,fnsllog10,
    fnsqlog10,fnslexp,fnsqexp,fnslrec,
    fnsqrec,fncvpower,fnvcpower,
    fncentropy,fngmillisec,fnmaxerror,
    fntimeel,fngamma,fnloggamma,fnbeta,
    fnlogbeta,fngammareg,fnbetareg,
    fnsinh,fncosh,fntanh,fnmathlastrc,
    fnmathlastec,fnmathoval,fnsignpower,
    fnhandle,fnncpvusin,fnncpvupow,
    fnbinomial,fnrehandle,fngamsver,
    fndelhandle,fntan,fnarccos,
    fnarcsin,fnarctan2,fnsleep,fnheapf,
    fncohandle,fngamsrel,fnpoly,
    fnlicensestatus,fnlicenselevel,fnheaplimit,
    fndummy} GamsFuncCode;


/** Gives the opcode of a GAMS nonlinear instruction.
 */
GamsOpCode getInstrOpCode(unsigned int instr);

/** Gives the address in a GAMS nonlinear instruction.
 * The address will be 0-based.
 */
int getInstrAddress(unsigned int instr);

#if 0
/** Reorders instructions such that they do not contain PushS, Popup, or Swap anymore.
 */
void reorderInstr(unsigned int* instr, int num_instr);
#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /*GAMSNLINSTR_H_*/
