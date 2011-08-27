/* Copyright (C) 2008 GAMS Development and others
   All Rights Reserved.
   This code is published under the Common Public License.

   $Id: GamsNLinstr.c 592 2008-11-12 14:11:00Z stefan $

   Author: Stefan Vigerske

   WARNING:
   The NL instruction (codes, format, semantics, etc.) are subject to change.
   Thus, there is no warranty that a code based on this file is going to work fine
   after changes in the GAMS I/O libraries as provided by GAMS.
   GAMS is NOT obligated to issue a warning when it changes the NL instructions.
*/

#include "GamsNLinstr.h"
#include <stdlib.h>

const char* GamsOpCodeName[MAXINS] = {
	"nlNoOp",
	"nlPushV",
	"nlPushI",
	"nlStore",
	"nlAdd",
	"nlAddV",
	"nlAddI",
	"nlAddL",
	"nlSub",
	"nlSubV",
	"nlSubI",
	"nlSubL",
	"nlMul",
	"nlMulV",
	"nlMulI",
	"nlMulL",
	"nlDiv",
	"nlDivV",
	"nlDivI",
	"nlDivL",
	"nlUMin",
	"nlUMinV",
	"nlSwap",
	"nlPushL",
	"nlPopL",
	"nlPopDeriv",
	"nlHeader",
	"nlUMinl",
	"nlStoreS",
	"nlPopDerivS",
	"nlEquScale",
	"nlEnd",
	"nlCallArg1",
	"nlCallArg2",
	"nlCallArgN",
	"nlFuncArgN",
	"nlPushS",
	"nlPopup",
	"nlArg",
	"nlMulIAdd",
	"nlPushZero",
	"nlMulPop1",
	"nlMulPop2",
	"nlMulPop",
	"nlAddPop", 
	"nlSubPop", 
	"nlGetConst", 
	"nlMulConst1", 
	"nlMulConst2", 
	"nlMulConst", 
	"nlNegLocal", 
	"nlGetLocal", 
	"nlSetLocal1", 
	"nlSetLocal2", 
	"nlSetLocal", 
	"nlGetGrad", 
	"nlPushIGrad", 
	"nlChk", 
	"nlAddO", 
	"nlPushO",
	"nlInvoc", 
	"nlStackIn"
};

/** Gives the opcode of a GAMS nonlinear instruction.
 */
GamsOpCode getInstrOpCode(unsigned int instr) {
	int iinstr = instr>>26;
/*	assert(iinstr < MAXINS); */
	return (GamsOpCode)iinstr;
}

/** Gives the address in a GAMS nonlinear instruction.
 * The address will be 0-based.
 */
int getInstrAddress(unsigned int instr) {
	return (instr & 67108863)-1;
}

void swapInstr(unsigned int* instr, int len1, int len2) {
	int moves = len1+len2;
	int first = 0;
	unsigned int tmp, dest, source;
	
	do {
		++first;
		tmp = instr[first];
		dest = first;
		source = first + len1;
		do {
			instr[dest] = instr[source];
			--moves;
			dest = source;
			if (source <= len2)
				source += len1;
			else
				source -= len2;
		} while(source!=first);
		instr[dest] = tmp;
		--moves;
	} while(moves>0);
}

/** Reorders instructions such that they do not contain PushS, Popup, or Swap anymore.
 */
void reorderInstr(unsigned int* instr, int num_instr) {
	int stacklen = 0;
	int j,k;
	int* instrpos = (int*)malloc(sizeof(int)*num_instr);
	instrpos[0] = 0;

	for (k=1; k<num_instr; ++k) {
		GamsOpCode opcode = getInstrOpCode(instr[k]);
		switch (opcode) {
			case nlUMinV:
			case nlPushV:
			case nlPushI:
			case nlPushZero:
				++stacklen;
				break;
			case nlCallArgN:
				stacklen -= getInstrAddress(instr[k-1]); /* getInstrAdress already decreases by 1 */
				break;
			case nlMulIAdd:
			case nlCallArg2:
			case nlStore:
			case nlStoreS:
			case nlAdd:
			case nlSub:
			case nlDiv:
			case nlMul:
				--stacklen;
				break;
			case nlSwap: {
				swapInstr(instr+instrpos[stacklen-2], instrpos[stacklen-1]-instrpos[stacklen-2], instrpos[stacklen]-instrpos[stacklen-1]);
				instrpos[stacklen-1] = instrpos[stacklen-2] - instrpos[stacklen-1] + instrpos[stacklen];
				instr[k] = ((unsigned int)nlArg)<<26; /* nlNoop should be just zero, but confused g2dHesRowStruct, so we take nlArg */
			} break;
			case nlPushS: {
				int len = getInstrAddress(instr[k])+1;
				int pushshift = instrpos[stacklen-len+1] - instrpos[stacklen-len];
/*				for (int i=0; i<=stacklen; ++i) std::clog << i << '\t' << instrpos[i] << std::endl;
				std::clog << "stacklen " << stacklen << " len " << len << std::endl;
				std::clog << "swap " << instrpos[stacklen-len] << ' ' << instrpos[stacklen-len+1] << ' ' << k-2 << std::endl;
				std::clog << "pushshift " << pushshift << std::endl;
*/				swapInstr(instr+instrpos[stacklen-len], pushshift, k-2-instrpos[stacklen-len+1]);
				for (j = stacklen-len+1; j<=stacklen; ++j)
					instrpos[j] -= pushshift;
				--instrpos[stacklen];
				instr[k-1] = ((unsigned int)nlArg)<<26;
				instr[k] = ((unsigned int)nlArg)<<26;
/*				for (int i=0; i<=stacklen; ++i) std::clog << i << '\t' << instrpos[i] << std::endl;
				for (int i=0; i<num_instr; ++i) std::clog << i << '\t' << GamsOpCodeName[getInstrOpCode(instr[i])] << std::endl;
*/				++stacklen;
			} break;
			case nlPopup: {
				stacklen -= getInstrAddress(instr[k])+1;
				instr[k] = ((unsigned int)nlArg)<<26;
			} break;
			default: ;
		}
		instrpos[stacklen] = k;
	}
	
	free(instrpos);
}
