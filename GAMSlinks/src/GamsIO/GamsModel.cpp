// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsModel.cpp 736 2009-09-04 15:08:34Z stefan $
//
// Authors:  Michael Bussieck, Stefan Vigerske

#include "GamsModel.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif
#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif
#ifdef HAVE_CASSERT
#include <cassert>
#else
#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#error "don't have header file for assert"
#endif
#endif
#ifdef HAVE_CLIMITS
#include <climits>
#else
#ifdef HAVE_ASSERT_H
#include <limits.h>
#else
#error "don't have header file for limits"
#endif
#endif

#include "iolib.h"
#include "dict.h"

static cntrec cntinfo;          /* control file information */

GamsModel::GamsModel(const char *cntrfile)
{
  gfinit ();
  gfrcnt (1, 0, &cntinfo, cntrfile);  // no NLP's
  gfappstx ();                        // append to status file (overwrite if dummy)

  // Sizes for memory allocation
  nCols_ = iolib.ncols;
  if (iolib.modin == PROC_MIP) {
  	nDCols_ = iolib.ndisc-iolib.nsemi; // count of discrete columns seem to include semiint and semicol
	  nSOS1_ = iolib.nosos1;
  	nSOS2_ = iolib.nosos2;
	  nSemiCon_ = iolib.nsemi+iolib.nsemii;
  } else {
  	nDCols_ = 0;
	  nSOS1_ = 0;
  	nSOS2_ = 0;
	  nSemiCon_ = 0;
  }
  nRows_ = iolib.nrows;
  nNnz_  = iolib.nnz;
  Allocate();

  // Can we reformulate the objective function?
  isReform_ = 
    iolib.reform           && // User request
    iolib.objEquType == EQ && // is EQ constraint
    iolib.objVarType == 0  && // is linear
    iolib.slpnzo     == 1  && // in exactly one row
    iolib.slpbno     == 0;    // variable unbounded

  
  // Some initializations
  ObjVal_   = 0.0;
  ObjBound_ = iolib.vlundf;
  ObjSense_ = (0 == iolib.idir) ? 1.0 : -1.0;
  ResUsed_  = 0.0;
  IterUsed_ = 0;
  DomUsed_  = 0;
  NodeUsed_ = 0;

  SolverStatus_ = SolverStatusNotSet;
  ModelStatus_ = ModelStatusNotSet;
}

void GamsModel::setInfinity(const double& SolverMInf, const double& SolverPInf) {
	// Solver infinity for infinity bounds
	iolib.usrminf = SolverMInf;
  iolib.usrpinf = SolverPInf;
}

double GamsModel::getMInfinity() const {
	return iolib.usrminf;
}

double GamsModel::getPInfinity() const {
	return iolib.usrpinf;
}

void GamsModel::Allocate()
{
  ColLb_   = new double[nCols_];
  ColUb_   = new double[nCols_];
  ColDisc_ = new bool[nCols_];
  SOSIndicator_ = new int[nCols_];
  ColSemiCon_ = new bool[nCols_];
  
  ObjCoef_ = new double[nCols_];

  RowSense_ = new char[nRows_];
  RowRhs_   = new double[nRows_];

  matStart_  = new int[nCols_+1];
  matRowIdx_ = new int[nNnz_];
  matValue_  = new double[nNnz_];

  RowLevel_     = new double[nRows_];
  RowMargin_    = new double[nRows_];
  RowBasis_     = new int[nRows_];
  RowIndicator_ = new int[nRows_];
  RowScale_     = new double[nRows_];
                           
  ColLevel_     = new double[nCols_];
  ColMargin_    = new double[nCols_];
  ColBasis_     = new int[nCols_];
  ColIndicator_ = new int[nCols_];
  ColPriority_  = new double[nCols_];
  ColScale_     = new double[nCols_];
}

// Destructor
GamsModel::~GamsModel()
{
  // Release memory
  delete[] ColLb_;
  delete[] ColUb_;
  delete[] ColDisc_;
  delete[] SOSIndicator_;
  delete[] ColSemiCon_;
  
  delete[] ObjCoef_;

  delete[] RowSense_;
  delete[] RowRhs_;

  delete[] matStart_;
  delete[] matRowIdx_;
  delete[] matValue_;

  delete[] RowLevel_;
  delete[] RowMargin_;
  delete[] RowBasis_;
  delete[] RowIndicator_;
  delete[] RowScale_;

  delete[] ColLevel_;
  delete[] ColMargin_;
  delete[] ColBasis_;
  delete[] ColIndicator_;
  delete[] ColPriority_;
  delete[] ColScale_;

  // Close IO system
  gfclos ();
}

void GamsModel::PrintOut(PrintMask mask, const char *msg)
{
  if (mask&LogMask && 0 != gfiolog) {
    fprintf (gfiolog, "%s\n", msg); fflush (gfiolog);
  }
  if (mask&StatusMask && 0 != gfiosta) 
    fprintf (gfiosta, "%s\n", msg);
}


#define EQ   0 // equalities
#define GT   1 // greater-equal
#define LT   2 // lower-equal
#define FR   3 // free rows

#define VARCON 0 // continuous
#define VARBIN 1 // binary
#define VARINT 2 // integer
#define VARSOS1 3 // sos of type 1
#define VARSOS2 4 // sos of type 2
#define VARSEMICON 5 // semicontinuous
#define VARSEMIINT 6 // semiinteger

void GamsModel::readMatrix() {
  int i,ip,j,jp,k,kp,nzcol,nltype;
  double val;
  rowRec_t rowRec;
  colRec_t colRec;
  
  ObjRhs_=0.0;
  for (i=0, ip=0; i<nRows_; ++i) {
    cioReadRow (&rowRec);
    if (isReform_ && i==iolib.slplro-1) {
      ObjRhs_ = rowRec.rdata[2];
      ObjScale_ = rowRec.rdata[4];
    } else {
    	RowLevel_[ip] = rowRec.rdata[1];
      RowRhs_[ip] = rowRec.rdata[2];
      RowMargin_[ip] = rowRec.rdata[3];
      RowScale_[ip] = rowRec.rdata[4];
      
      RowBasis_[ip] = rowRec.idata[2];
      switch (rowRec.idata[1]) {	/* row sense */
      case EQ:
        RowSense_[ip] = 'E';
        break;
      case LT:
        RowSense_[ip] = 'L';
        break;
      case GT:
        RowSense_[ip] = 'G';
        break;
      case FR:
      	RowSense_[ip] = 'N';
			  break;
      default:
        PrintOut(AllMask, "\n*** Unhandled Row Type. Allowed row types =L=, =G=, =E=, =N=\n");
        exit(EXIT_FAILURE); // We should throw an exception
      }
      ip++;
    }
  }

  for (j=0, jp=0, kp=0; j<nCols_; ++j) {
    cioReadCol (&colRec);
    nzcol = colRec.idata[1];
    ObjCoef_[jp] = 0.0; // initialize objective row
    if (isReform_ && j==iolib.iobvar) {
      assert(1==nzcol);
      gfrcof (&zCoef_, &i, &nltype);
      assert(0==nltype);
      ObjScale_/=colRec.cdata[6];
    }
    else {
      ColLb_[jp] = colRec.cdata[1];
      ColLevel_[jp] = colRec.cdata[2];
      ColUb_[jp] = colRec.cdata[3];
      ColPriority_[jp] = colRec.cdata[4];
      ColMargin_[jp] = colRec.cdata[5];
      ColScale_[jp] = colRec.cdata[6];
      
      // change type of trivial semicon/semiint variables to continuous/integer
      if (colRec.idata[3] == VARSEMICON && ColLb_[jp]==0.) {
      	colRec.idata[3] = VARCON;
      	--nSemiCon_;
        PrintOut(LogMask, "Warning: Changed semicontinuous variable with lower bound 0. to continuous variable.");
      }
      if (colRec.idata[3]==VARSEMIINT && (ColLb_[jp]==0. || ColLb_[jp]==1.)) {
      	colRec.idata[3] = VARINT;
      	--nSemiCon_;
        PrintOut(LogMask, "Warning: Changed semiinteger variable with lower bound 0. or 1. to continuous variable.");
      }
      
      ColBasis_[jp] = colRec.idata[2];
      switch (colRec.idata[3]) {	/* variable type */
      case VARCON:
        ColDisc_[jp]=false;
				ColSemiCon_[jp]=false;
        SOSIndicator_[jp]=0;
        break;
      case VARINT:
      case VARBIN:
        ColDisc_[jp]=true;
				ColSemiCon_[jp]=false;
        SOSIndicator_[jp]=0;
        break;
      case VARSOS1:
        ColDisc_[jp]=false;
				ColSemiCon_[jp]=false;
        SOSIndicator_[jp]=colRec.idata[4];
      	break;
      case VARSOS2:
        ColDisc_[jp]=false;
				ColSemiCon_[jp]=false;
        SOSIndicator_[jp]=-colRec.idata[4];
      	break;
      case VARSEMICON:
        ColDisc_[jp]=false;
				ColSemiCon_[jp]=true;
        SOSIndicator_[jp]=0;
        break;
      case VARSEMIINT:
        ColDisc_[jp]=true;
				ColSemiCon_[jp]=true;
        SOSIndicator_[jp]=0;
        break;
      default:
        PrintOut(AllMask, "\n*** Unhandled column type. Allowed column types: continuous, binary, integer, sos1, sos2, semicontinuous, semiinteger\n");
        exit(EXIT_FAILURE); // We should throw an exception
      }
      matStart_[jp] = kp;
      for (k=0;  k<nzcol;  k++) {
        gfrcof (&val, &i, &nltype); i--;
        assert(0==nltype);
        if (isReform_ && i==iolib.slplro-1) 
          ObjCoef_[jp] = val;
        else {
          if (isReform_ && i>iolib.slplro-1)
            i--;
          matRowIdx_[kp] = i;
          matValue_[kp] = val;
          kp++;
        }
      }
      jp++;
    }
  }
  matStart_[jp] = kp;

  nNnz_ = kp;
  nCols_ = jp;
  nRows_ = ip;

  if (isReform_) {
    for (j=0; j<nCols_; j++) {
      ObjCoef_[j] /= -zCoef_;
    }
    ObjRhs_ /= -zCoef_;
  }
  else
    ObjCoef_[iolib.iobvar] = 1.0;
}

int GamsModel::matSqueezeZeros() {
	int col, k;
	int shift=0;
	for (col=0; col<nCols_; ++col) {
		matStart_[col+1]-=shift;
		k=matStart_[col];
		while (k<matStart_[col+1]) {
			matValue_[k]=matValue_[k+shift];
			matRowIdx_[k]=matRowIdx_[k+shift];
			if (matValue_[k]==0) {
				++shift;
				--matStart_[col+1];
			} else {
				++k;
			}
		}		
	}
	nNnz_-=shift;
	
	return shift;
}

bool GamsModel::isLP() const {
	return (0==nDCols() && 0==nSOS1() && 0==nSOS2() && 0==nSemiContinuous());	
}


void GamsModel::TimerStart() {
  startTime_ = gfclck();
}

double GamsModel::SecondsSinceStart() {
  return gfclck() - startTime_;
}

const char* GamsModel::getOptionfile() {
	if (iolib.useopt)
		return iolib.flnopt;
	else
		return NULL;
}

const char* GamsModel::getSystemDir() {
	return iolib.gsysdr;	
}

double GamsModel::getResLim() {
  return iolib.reslim;
}

int GamsModel::getIterLim() {
  return iolib.itnlim;
}

int GamsModel::getNodeLim() {
  return (iolib.nodlim==0)? INT_MAX:iolib.nodlim;
}

int GamsModel::getGamsInteger(int i) {
  return (i>=0 && i<5)? iolib.iopt[i]:0;
}

int GamsModel::getGamsInteger(int i, int j) {
  if (i<0 || i>4) return 0;
  else if (j<0 || j>15) return 0;
  else return (iolib.iopt[i]>>j)%2;
}

double GamsModel::getOptCA() {
  return iolib.optca;
}

double GamsModel::getOptCR() {
  return iolib.optcr;
}

double GamsModel::getCheat() {
	if (iolib.icheat)
		return iolib.cheat;
	else
		return 0.;
}

int GamsModel::getSysOut() {
  return iolib.sysout;
}

double GamsModel::getCutOff() {
  if (iolib.icutof)
    return iolib.cutoff;
  else // plus infinity for min. problem; minus infinity for max. problem
    return (ObjSense_>0)? iolib.usrpinf:iolib.usrminf;
}

bool GamsModel::getScaleOption() {
	return iolib.iscopt;
}

bool GamsModel::getPriorityOption() {
	return iolib.priots;
}

bool GamsModel::getHaveAdvancedBasis() {
	return iolib.ignbas; /* ignbas = 1 means that we should NOT ignore the advanced basis */
}

void GamsModel::setStatus(const SolverStatus& newSolverStatus, 
                          const ModelStatus& newModelStatus) {
  if (newSolverStatus > SolverStatusNotSet && 
      newSolverStatus < LastSolverStatus)
    SolverStatus_ = newSolverStatus;
  if (newModelStatus > ModelStatusNotSet && 
      newModelStatus < LastModelStatus)
    ModelStatus_ = newModelStatus;
}

GamsModel::ModelStatus GamsModel::getModelStatus() {
  assert(ModelStatus_ > ModelStatusNotSet && ModelStatus_ < LastModelStatus);
  return ModelStatus_;
}

GamsModel::SolverStatus GamsModel::getSolverStatus() {
  assert(SolverStatus_ > SolverStatusNotSet && SolverStatus_ < LastSolverStatus);
  return SolverStatus_;
}

void GamsModel::setSolution(const double *ColLevel, const double *ColMargin, 
                            const int *ColBasis, const int *ColIndicator,
                            const double *RowLevel, const double *RowMargin, 
                            const int *RowBasis, const int *RowIndicator) {

  // Write header
  assert(SolverStatus_ > SolverStatusNotSet && SolverStatus_ < LastSolverStatus);
  assert(ModelStatus_ > ModelStatusNotSet && ModelStatus_ < LastModelStatus);
  gfwsta (ModelStatus_, SolverStatus_, IterUsed_, ResUsed_, ObjVal_, DomUsed_);

  if (ModelStatus_ == Optimal || ModelStatus_ == IntegerSolution) { // and other cases where we want to write a solution
    int i,j,basis,indicator;
    double level, marginal;
    
    // Copy into our space
    if (ColLevel != NULL && ColLevel !=ColLevel_)
      for (j=0; j<nCols_; j++) ColLevel_[j] = ColLevel[j];
    if (ColMargin != NULL && ColMargin !=ColMargin_)
      for (j=0; j<nCols_; j++) ColMargin_[j] = ColMargin[j];
    if (ColBasis != NULL && ColBasis !=ColBasis_)
      for (j=0; j<nCols_; j++) ColBasis_[j] = ColBasis[j];
    if (ColIndicator != NULL && ColIndicator !=ColIndicator_)
      for (j=0; j<nCols_; j++) ColIndicator_[j] = ColIndicator[j];

    if (RowLevel != NULL && RowLevel !=RowLevel_)
      for (i=0; i<nRows_; i++) RowLevel_[i] = RowLevel[i];
    if (RowMargin != NULL && RowMargin !=RowMargin_)
      for (i=0; i<nRows_; i++) RowMargin_[i] = RowMargin[i];
    if (RowBasis != NULL && RowBasis !=RowBasis_)
      for (i=0; i<nRows_; i++) RowBasis_[i] = RowBasis[i];
    if (RowIndicator != NULL && RowIndicator !=RowIndicator_)
      for (i=0; i<nRows_; i++) RowIndicator_[i] = RowIndicator[i];

    for (i=0; i<nRows_+1; i++) {
      if (isReform_ && i == iolib.slplro-1) { // If old objective
        level = -ObjRhs_*zCoef_;
        marginal = 1/zCoef_;
        basis=NonBasicLower; indicator=0;
        gfwrow (level, marginal, basis, indicator);
      }
      if (i<nRows_) {
        level = 0.0; marginal = 0.0; basis = SuperBasic; indicator = RowNormal;
        if (RowLevel != NULL)      level     = RowLevel_[i];
        if (RowMargin != NULL)     marginal  = RowMargin_[i];
        if (RowBasis != NULL)      basis     = RowBasis_[i];
        if (RowIndicator != NULL)  indicator = RowIndicator_[i];
        gfwrow (level, marginal, basis, indicator);
      }
    }
      
    for (j=0; j<nCols_+1; j++) {
      if (isReform_ && j == iolib.iobvar) { // If old objective variable
        level = ObjVal_;
        marginal = 0.0;
        basis=Basic; indicator=0;
        gfwcol (level, marginal, basis, indicator);
      }
      if (j<nCols_) {
        level = 0.0; marginal = 0.0; basis = SuperBasic; indicator = VarNormal;
        if (ColLevel != NULL)      level     = ColLevel_[j];
        if (ColMargin != NULL)     marginal  = ColMargin_[j];
        if (ColBasis != NULL)      basis     = ColBasis_[j];
        if (ColIndicator != NULL)  indicator = ColIndicator_[j];
        gfwcol (level, marginal, basis, indicator);
      }
    }
  }

	gfwbnd(ObjBound_);
	gfwnod(NodeUsed_);
	gfcsol(); // close solution file 
}
