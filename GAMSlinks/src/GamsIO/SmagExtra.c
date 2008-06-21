/* Copyright (C) 2008 GAMS Development and others
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id$

 Author: Stefan Vigerske
*/

#include "SmagExtra.h"

#include <stdlib.h>
#include <assert.h>

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

int smagEvalObjGradProd(smagHandle_t prob, double* x, double* d, double* f, double* df_d) {
  gmsCFRec_t *gms;
  smagObjGradRec_t *og;
  strippedNLdata_t *snl;
  double* x_, *d_;
  int* S2G_;
  int gi, sj;
  int numErr;

  assert(x);
  assert(d);
  gms = &prob->gms;
  snl = &prob->snlData;

  if (0 == prob->objColCountNL) { /* linear objective */
    *f = 0;
    *df_d = 0;
    for (og = prob->objGrad;  og;  og = og->next) {
      *f += og->dfdj * x[og->j];
      *df_d += og->dfdj * d[og->j];
    }
    *f -= gms->grhs[gms->slplro-1] * prob->gObjFactor;
    return 0;
  }

  /* transform x and d into original gams space, storing in prob->gx and snl->dfdx */
  x_ = x;
  d_ = d;
  S2G_ = prob->colMapS2G;
  for (sj = prob->colCount; sj; --sj, ++x_, ++d_, ++S2G_) {
    prob->gx[*S2G_] = *x_;
    snl->dfdx[*S2G_] = *d_; /* we just capture this memory for the next G2D call to avoid an extra allocation/free */
  }

  numErr = 0;
  gi = prob->gObjRow;
  
  G2DGRADVPRODEVALX (prob->gx, snl->dfdx, f, df_d,
  		snl->instr + snl->startIdx[gi] - 1, snl->numInstr + gi, snl->nlCons, &numErr);

  if (numErr)
    return numErr;

  *f *= prob->gObjFactor;
  *df_d *= prob->gObjFactor;
  for (og = prob->objGrad;  og;  og = og->next) {
    if (1==og->degree) {		/* linear term  in f(x) */
      *f += og->dfdj * x[og->j];
      *df_d += og->dfdj * d[og->j];
    }
  }

  *f -= gms->grhs[gms->slplro-1] * prob->gObjFactor;
    
  return 0;
}

#ifndef DONT_DEFINE_STRICMP
/* bch.o uses a reference to stricmp which is defined in iolib.
 * Since we usually do not like to link to iolib, we define our own stricmp function here.
 * Either we use strcasecmp, if available, or we use gcdstricmp from GAMS dictread.o 
 */
#ifndef HAVE_STRICMP
#ifdef HAVE_STRCASECMP
#include <strings.h>
int stricmp(const char* s1, const char* s2) {
	return strcasecmp(s1, s2);
}
#else
int gdcstricmp(const char* s1, const char* s2);
int stricmp(const char* s1, const char* s2) {
	return gcdstricmp(s1, s2);
}
#endif
#endif
#endif
