// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#include <cassert>
#include <cstdio>
#include <limits.h>

#include "GamsModel.hpp"
#include "iolib.h"
#include "dict.h"

static cntrec cntinfo;          /* control file information */

GamsModel::GamsModel(const char *cntrfile, const double SolverInf)
{
  gfinit ();
  gfrcnt (1, 0, &cntinfo, cntrfile);  // no NLP's for the moment
  gfappstx ();                        // append to status file (overwrite if dummy)

  // Sizes for memory allocation
  nCols_ = iolib.ncols;
  nSOS1_ = iolib.nosos1;
  nSOS2_ = iolib.nosos2;
  nRows_ = iolib.nrows;
  nNnz_  = iolib.nnz;
  Allocate();

  // Solver infinity for infinity bounds
  iolib.usrpinf = SolverInf;
  iolib.usrminf = -SolverInf;

  // Can we reformulate the objective function?
  isReform_ = 
    iolib.reform           && // User request
    iolib.objEquType == EQ && // is EQ constraint
    iolib.objVarType == 0  && // is linear
    iolib.slpnzo     == 1  && // in exactly one row
    iolib.slpbno     == 0;    // variable unbounded

  
  // Some initializations
  ObjVal_   = 0.0;
  ResUsed_  = 0.0;
  IterUsed_ = 0;
  DomUsed_  = 0;

  SolverStatus_ = SolverStatusNotSet;
  ModelStatus_ = ModelStatusNotSet;

  ReadMatrix();
  
	dict=NULL;
	if (iolib.dictFileWritten) {
//		fprintf(gfiolog, "Reading dictionary file %s of version %d.\n", iolib.flndic, iolib.dictVersion);
		if (gcdLoad(&dict, iolib.flndic, iolib.dictVersion)) {
			PrintOut(ALLMASK, "Error reading dictionary file.\n");
			dict=NULL;
		}
	}

}

void GamsModel::Allocate()
{
  ColLb_   = new double[nCols_];
  ColUb_   = new double[nCols_];
  ColStat_ = new int[nCols_];
  ColDisc_ = new int[nCols_];
  SOSIndicator_ = new int[nCols_];
  
  ObjCoef_ = new double[nCols_];

  RowSense_ = new char[nRows_];
  RowRhs_   = new double[nRows_];
  RowStat_  = new int[nRows_];

  matStart_  = new int[nCols_+1];
  matRowIdx_ = new int[nNnz_];
  matValue_  = new double[nNnz_];

  RowLevel_     = new double[nRows_];
  RowMargin_    = new double[nRows_];
  RowBasis_     = new int[nRows_];
  RowIndicator_ = new int[nRows_];
                           
  ColLevel_     = new double[nCols_];
  ColMargin_    = new double[nCols_];
  ColBasis_     = new int[nCols_];
  ColIndicator_ = new int[nCols_];
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

void GamsModel::ReadMatrix()
{
  int i,ip,j,jp,k,kp,nzcol,nltype;
  double val;
  rowRec_t rowRec;
  colRec_t colRec;
  
  ObjRhs_=0.0;
  for (i=0, ip=0; i<nRows_; ++i) {
    cioReadRow (&rowRec);
    if (isReform_ && i==iolib.slplro-1)
      ObjRhs_ = rowRec.rdata[2];
    else {
      RowRhs_[ip] = rowRec.rdata[2];
      RowStat_[ip] = rowRec.idata[2];
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
        PrintOut(ALLMASK, "\n*** Unhandled Row Type. Allowed row types =L=, =G=, =E=, =N=\n");
        exit(0); // We should throw an exception
      }
      ip++;
    }
  }

  nDCols_ = 0;
  for (j=0, jp=0, kp=0; j<nCols_; ++j) {
    cioReadCol (&colRec);
    nzcol = colRec.idata[1];
    ObjCoef_[jp] = 0.0; // initialize objective row
    if (isReform_ && j==iolib.iobvar) {
      assert(1==nzcol);
      gfrcof (&zCoef_, &i, &nltype);
      assert(0==nltype);
    }
    else {
      ColLb_[jp] = colRec.cdata[1];
      ColUb_[jp] = colRec.cdata[3];
      ColStat_[jp] = colRec.idata[2];
      switch (colRec.idata[3]) {	/* variable type */
      case VARCON:
        ColDisc_[jp]=0;
        SOSIndicator_[jp]=0;
        break;
      case VARINT:
      case VARBIN:
        ColDisc_[jp]=1;
        SOSIndicator_[jp]=0;
        nDCols_++;
        break;
      case VARSOS1:
        ColDisc_[jp]=0;
        SOSIndicator_[jp]=colRec.idata[4];
      	break;
      case VARSOS2:
        ColDisc_[jp]=0;
        SOSIndicator_[jp]=-colRec.idata[4];
      	break;
      default:
        PrintOut(ALLMASK, "\n*** Unhandled column type. Allowed column types: continuous, binary, integer, sos1, sos2\n");
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

  ObjSense_ = (0 == iolib.idir) ? 1.0 : -1.0;
  if (isReform_) {
    for (j=0; j<nCols_; j++) {
      ObjCoef_[j] /= -zCoef_;
    }
    ObjRhs_ /= -zCoef_;
  }
  else
    ObjCoef_[iolib.iobvar] = 1.0;
}

void GamsModel::TimerStart() {
  startTime_ = gfclck();
}

double GamsModel::SecondsSinceStart() {
  return gfclck() - startTime_;
}

const char* GamsModel::getOptionfile() {
	return iolib.flnopt;	
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

int GamsModel::getGamsSwitch(int i, int j) {
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

int GamsModel::getSysOut() {
  return iolib.sysout;
}

double GamsModel::getCutOff() {
  if (iolib.icutof)
    return iolib.cutoff;
  else // plus infinity for min. problem; minus infinity for max. problem
    return (ObjSense_>0)? iolib.usrpinf:iolib.usrminf;
}

void GamsModel::setStatus(const GamsModelSolverStatus SolverStatus, 
                          const GamsModelModelStatus ModelStatus) {
  if (SolverStatus > SolverStatusNotSet && 
      SolverStatus < LastSolverStatus)
    SolverStatus_ = SolverStatus;
  if (ModelStatus > ModelStatusNotSet && 
      ModelStatus < LastModelStatus)
    ModelStatus_ = ModelStatus;
}

int GamsModel::getModelStatus() {
  assert(ModelStatus_ > ModelStatusNotSet && ModelStatus_ < LastModelStatus);
  return ModelStatus_;
}

void GamsModel::setSolution(const double *ColLevel, const double *ColMargin, 
                            const int *ColBasis, const int *ColIndicator,
                            const double *RowLevel, const double *RowMargin, 
                            const int *RowBasis, const int *RowIndicator) {

  // Write header
  assert(SolverStatus_ > SolverStatusNotSet && SolverStatus_ < LastSolverStatus);
  assert(ModelStatus_ > ModelStatusNotSet && ModelStatus_ < LastModelStatus);
  gfwsta (ModelStatus_, SolverStatus_, IterUsed_, ResUsed_, ObjVal_, DomUsed_);
  // gfcsol ();

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
        level = 0.0; marginal = 0.0; basis = SuperBasic; indicator = 0;
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
        level = 0.0; marginal = 0.0; basis = 1; indicator = 0;
        if (ColLevel != NULL)      level     = ColLevel_[j];
        if (ColMargin != NULL)     marginal  = ColMargin_[j];
        if (ColBasis != NULL)      basis     = ColBasis_[j];
        if (ColIndicator != NULL)  indicator = ColIndicator_[j];
        gfwcol (level, marginal, basis, indicator);
      }
    }
  }
}

char* GamsModel::RowName(int rownr, char *buffer, int bufLen) {
	if (!dict) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

  if (rownr<0 || rownr >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, rownr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return ConstructName(buffer, bufLen, lSym, uelIndices, nIndices); 
}

char* GamsModel::ColName(int colnr, char *buffer, int bufLen) {
	if (!dict) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

  if (colnr < 0 || colnr >= gcdNCols(dict)) return NULL;
  if (gcdColUels(dict, colnr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return ConstructName(buffer, bufLen, lSym, uelIndices, nIndices);
}

char* GamsModel::ConstructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices) {
	if (bufLen<=0)
		return NULL;
  char
    quote,
    *targ,
    *end,
    *s,
    tbuf[32];
	
  if ((s=gcdSymName (dict, lSym, tbuf, 32)) == NULL) return NULL;

  targ = buffer;
  end = targ + bufLen-1; // keep one byte for a closing \0

  while (targ < end && *s != '\0') *targ++ = *s++;

  if (0 == nIndices || targ==end) {
    *targ = '\0';
    return buffer;
  }

  *targ++ = '(';
  for (int k=0;  k<nIndices && targ < end;  k++) {
    s = gcdUelLabel (dict, uelIndices[k], tbuf, 32, &quote);
    if (NULL == s) return NULL;

    if (' ' != quote) *targ++ = quote;
    while (targ < end && *s != '\0') *targ++ = *s++;
    if (targ == end) break;
    if (' ' != quote) {
    	*targ++ = quote;
	    if (targ == end) break;
    }
		*targ++ = (k+1==nIndices) ? ')' : ',';
  }
  *targ = '\0';

  return buffer;
}

// Destructor
GamsModel::~GamsModel()
{
  // Release memory
  delete[] ColLb_;
  delete[] ColUb_;
  delete[] ColStat_;
  delete[] ColDisc_;
  delete[] SOSIndicator_;
  
  delete[] ObjCoef_;

  delete[] RowSense_;
  delete[] RowRhs_;
  delete[] RowStat_;

  delete[] matStart_;
  delete[] matRowIdx_;
  delete[] matValue_;

  delete[] RowLevel_;
  delete[] RowMargin_;
  delete[] RowBasis_;
  delete[] RowIndicator_;

  delete[] ColLevel_;
  delete[] ColMargin_;
  delete[] ColBasis_;
  delete[] ColIndicator_;

	// Close dictionary
	if (dict) gcdFree(dict);
  // Close IO system
  gfclos ();
}

void 
GamsModel::PrintOut(int mask, const char *msg)
{
  if (mask&LOGMASK && 0 != gfiolog) {
    fprintf (gfiolog, "%s\n", msg); fflush (gfiolog);
  }
  if (mask&STATUSMASK && 0 != gfiosta) 
    fprintf (gfiosta, "%s\n", msg);
}

