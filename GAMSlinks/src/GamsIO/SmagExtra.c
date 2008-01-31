/* Copyright (C) 2008
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id: SmagJournal.cpp 298 2008-01-04 14:03:20Z stefan $

 Author: Stefan Vigerske
*/

#include "SmagExtra.h"

#include <stdlib.h>

#include "g2dexports.h"

int smagSingleHessians(smagHandle_t prob, int* hesRowIdx, int* hesColIdx, double* hesValue, int hesSize, int* rowStart) {
  gmsCFRec_t *gms;
  strippedNLdata_t *snl;
  int j;

	int hesHeadPtr, hesTailPtr, hesNumElem;
	int jacHeadPtr, jacTailPtr, jacNumElem;
	int hesLagSiz, jacSiz;
	int* hesRowCL; int* hesRowRW; int* hesRowNX; double* hesRowVal;
	int* jacVR; int* jacNX; double* jacVal;
	int extraMemMax;
	int numErr;
	
  int srownr;  /* at which smag row we currently are */
	int hesCount; /* where we currently are in the user provided arrays */

  if (prob->colCountNL == 0) {
  	for (j=0; j<=smagRowCount(prob)+1; ++j)
  		rowStart[j]=0;
  	return 0;
  }
	
  gms = &prob->gms;
  snl = &prob->snlData;
	extraMemMax=0;
	numErr=0;

	/* g2dData.hesOneLenMax would be sufficient, but I do not get easy access to g2dData here and it would assume that smagHessInit has been called before.
	  Or should we just trust the user and use hesSize?
	*/
	hesLagSiz=(int)(gms->workFactor * 10 * gms->nnz);
	hesRowCL=(int*)malloc(hesLagSiz*sizeof(int));
	hesRowRW=(int*)malloc(hesLagSiz*sizeof(int));
	hesRowNX=(int*)malloc(hesLagSiz*sizeof(int));
	jacSiz=snl->maxins;
	jacNX=(int*)malloc(jacSiz*sizeof(int));
	jacVR=(int*)malloc(jacSiz*sizeof(int));
	
	if (hesValue!=NULL) {
		hesRowVal=(double*)malloc(hesLagSiz*sizeof(double));
		jacVal=(double*)malloc(jacSiz*sizeof(double));
	} else {
		hesRowVal=NULL;
		jacVal=NULL;
	}
	
	G2DINITJACLIST(jacNX, &jacSiz);
	G2DINITHESLIST(hesRowNX, &hesLagSiz);

	hesCount=0;
	for (srownr=0; srownr<=smagRowCount(prob); ++srownr) { /* all smag rows plus objective */
		int grownr; /* row index in gams space */
		
		if (srownr==smagRowCount(prob)) { /* objective */
			grownr=gms->slplro-1;
		} else { /* usual row */
			grownr=prob->rowMapS2G[srownr];
		}
		
		rowStart[srownr]=hesCount;
		
		G2DREINITJACLIST(jacNX, &jacSiz);
		G2DREINITHESLIST(hesRowNX, &hesLagSiz);

		/* printf("start index row %d: %d\n", srownr, snl->startIdx[grownr]); */
    if (snl->startIdx[grownr]==0) {
    	/* printf("no nonlinearities in row %d\n", grownr); */
      continue;
    }
    if (hesValue!=NULL) {
    	double rowlevel, rowscale;
    	
      G2DFUNCEVAL0 (gms->gxl, &rowlevel,
  		  snl->s, snl->instr + snl->startIdx[grownr]-1, &(snl->numInstr[grownr]), snl->nlCons,
  		  &rowscale, &numErr);
    	
    	if (numErr==0)
    		G2DHESROWVAL(gms->gxl, &rowlevel, snl->instr+snl->startIdx[grownr]-1, snl->nlCons, &numErr,
    			&hesHeadPtr, &hesTailPtr, &hesNumElem,
    			&jacHeadPtr, &jacTailPtr, &jacNumElem,
    			jacVR, jacNX, jacVal,
    			hesRowCL, hesRowRW, hesRowNX, hesRowVal);
    	
      if (numErr>0) {
      	printf("Error %d in gams row %d\n", numErr, grownr);
      	break;
      }
   	
    } else {
    	/* startIdx is 1-based */
    	G2DHESROWSTRUCT(snl->instr+snl->startIdx[grownr]-1,
    			&hesHeadPtr, &hesTailPtr, &hesNumElem,
    			&jacHeadPtr, &jacTailPtr, &jacNumElem,
    			jacVR, jacNX,
    			hesRowCL, hesRowRW, hesRowNX,
    			&extraMemMax);
    }

  	for (j = 1; j; j = hesRowNX[j-1]) { /* hesRowNX is 1-based, so now is also j */
  		/* printf("%d: col %d \t row %d", j, hesRowCL[j-1], hesRowRW[j-1]);
  		if (hesValue!=NULL) printf("\t val %g\n", hesRowVal[j-1]);
  		else printf("\n");
			*/
  		hesRowIdx[hesCount]=prob->colMapG2S[hesRowCL[j-1]-1];
  		hesColIdx[hesCount]=prob->colMapG2S[hesRowRW[j-1]-1];
  		if (hesValue!=NULL)
    		hesValue[hesCount]=hesRowVal[j-1];
     	++hesCount;
     	
  		if (hesCount>=hesSize) {
  			numErr=-1;
  			break;
  		}
		}

  }
	
	if (numErr==0) {
		rowStart[srownr]=hesCount;
  
/*		printf("Success: G2DHESROWSTRUCT finished.\n");
*/	}

	free(hesRowCL);
	free(hesRowRW);
	free(hesRowNX);
	free(jacNX);
	free(jacVR);
	if (hesValue!=NULL) {
		free(hesRowVal);
		free(jacVal);
	}

  return numErr;
}
