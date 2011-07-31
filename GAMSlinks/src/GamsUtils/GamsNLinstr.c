/* Copyright (C) 2008-2011 GAMS Development and others
   All Rights Reserved.
   This code is published under the Eclipse Public License.

   Author: Stefan Vigerske

   WARNING:
   The NL instruction (codes, format, semantics, etc.) are subject to change.
   Thus, there is no warranty that a code based on this file is going to work fine
   after changes in the GAMS I/O libraries as provided by GAMS.
   GAMS is NOT obligated to issue a warning when it changes the NL instructions.
 */

#include "GamsNLinstr.h"

const char* GamsOpCodeName[MAXINS] =
{
   "nlNoOp",
   "nlPushV",
   "nlPushI",
   "nlStore",
   "nlAdd",
   "nlAddV",
   "nlAddI",
   "nlSub",
   "nlSubV",
   "nlSubI",
   "nlMul",
   "nlMulV",
   "nlMulI",
   "nlDiv",
   "nlDivV",
   "nlDivI",
   "nlUMin",
   "nlUMinV",
   "nlHeader",
   "nlEnd",
   "nlCallArg1",
   "nlCallArg2",
   "nlCallArgN",
   "nlFuncArgN",
   "nlMulIAdd",
   "nlPushZero",
   "nlChk",
   "nlAddO",
   "nlPushO",
   "nlInvoc",
   "nlStackIn"
};
