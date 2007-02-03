// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
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
#error "don't have header file for assert"
#endif
#endif

#include "iolib.h"
#include "dict.h"
#include "optcc.h"

#if defined(_WIN32)
# define snprintf _snprintf
#endif

static cntrec cntinfo;          /* control file information */

GamsModel::GamsModel(const char *cntrfile, const double SolverMInf, const double SolverPInf)
{
  gfinit ();
  gfrcnt (1, 0, &cntinfo, cntrfile);  // no NLP's
  gfappstx ();                        // append to status file (overwrite if dummy)

  // Sizes for memory allocation
  nCols_ = iolib.ncols;
  nSOS1_ = iolib.nosos1;
  nSOS2_ = iolib.nosos2;
  nRows_ = iolib.nrows;
  nNnz_  = iolib.nnz;
  Allocate();

  // Solver infinity for infinity bounds
	if (SolverMInf!=0) iolib.usrminf = SolverMInf;
  if (SolverPInf!=0) iolib.usrpinf = SolverPInf;

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
		if (gcdLoad(&dict, iolib.flndic, iolib.dictVersion)) {
			PrintOut(AllMask, "Error reading dictionary file.\n");
			dict=NULL;
		}
	}

	optionshandle=NULL;
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
GamsModel::PrintOut(PrintMask mask, const char *msg)
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
        PrintOut(AllMask, "\n*** Unhandled Row Type. Allowed row types =L=, =G=, =E=, =N=\n");
        exit(EXIT_FAILURE); // We should throw an exception
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
        PrintOut(AllMask, "\n*** Unhandled column type. Allowed column types: continuous, binary, integer, sos1, sos2\n");
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

void GamsModel::TimerStart() {
  startTime_ = gfclck();
}

double GamsModel::SecondsSinceStart() {
  return gfclck() - startTime_;
}

const char* GamsModel::getOptionfile() {
	return iolib.flnopt;	
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

int GamsModel::getSysOut() {
  return iolib.sysout;
}

double GamsModel::getCutOff() {
  if (iolib.icutof)
    return iolib.cutoff;
  else // plus infinity for min. problem; minus infinity for max. problem
    return (ObjSense_>0)? iolib.usrpinf:iolib.usrminf;
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


bool GamsModel::ReadOptionsDefinitions(const char* solvername) {
	/* Get the Option File Handling set up */
	if (NULL==optionshandle)
		optCreate(&optionshandle);
	if (NULL==optionshandle) {
		PrintOut(AllMask, "\n*** Could not create optionfile handle."); 
		return false;
	}

	char buffer[255];
	if (snprintf(buffer, 255, "%sopt%s.def", getSystemDir(), solvername)>=255) {
		PrintOut(AllMask, "\n*** Path to GAMS system directory too long.\n");
		return false;
	}
	if (optReadDefinition(optionshandle,buffer)) {
		int itype; 
		for (int i=1; i<=optMessageCount(optionshandle); ++i) {
			optGetMessage(optionshandle, i, buffer, &itype);
			PrintOut(AllMask, buffer);
		}
		optClearMessages(optionshandle);
		return false;
	}   
	optEOLOnlySet(optionshandle,1);
	
	return true;
}

bool GamsModel::ReadOptionsFile() {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::ReadOptionsFile: Optionfile handle not initialized.");
		return false;
	}
	/* Read option file */
	if (iolib.useopt) {
	  optEchoSet(optionshandle, 1);
	  optReadParameterFile(optionshandle, getOptionfile());
	  if (optMessageCount(optionshandle)) {
		  char buffer[255];
		  int itype;
		  for (int i=1; i<=optMessageCount(optionshandle); ++i) {
	  	  optGetMessage(optionshandle, i, buffer, &itype);
	    	if (itype<=optMsgFileLeave || itype==optMsgUserError)
	    		PrintOut(AllMask, buffer);
		  }
		  optClearMessages(optionshandle);
		}   
		optEchoSet(optionshandle, 0);
	}

	return true;
}


bool GamsModel::optDefined(const char *optname) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optDefined: Optionfile handle not initialized.");
		return false;
	}
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType;

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    return isDefined;
  } else {
  	char buffer[255];
  	snprintf(buffer, 255, "*** Internal Error. Unknown option %s", optname);
		PrintOut(AllMask, buffer);
    return false;
  }
}

//bool GamsModel::optDefinedRecent(const char *optname) {
//	if (NULL==optionshandle) {
//		PrintOut(AllMask, "GamsModel::optDefinedRecent: Optionfile handle not initialized.");
//		return false;
//	}
//  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
//
//  if (optFindStr (optionshandle, optname, &i, &refNum)) {
//    optGetInfoNr (optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
//    return isDefinedRecent;
//  } else {
//  	char buffer[255];
//  	snprintf(buffer, 255, "*** Internal Error. Unknown option %s", optname);
//		PrintOut(AllMask, buffer);
//    return false;
//  }
//}

int GamsModel::optGetInteger(const char *optname) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optGetInteger: Optionfile handle not initialized.");
		return 0;
	}
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
    	snprintf(sval, 255, "*** Internal Error. Option %s is not an integer (it is %d)", optname, dataType);
    	PrintOut(AllMask, sval);
      return 0;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return ival;
  } 

 	snprintf(sval, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, sval);
	return 0;
}

double GamsModel::optGetDouble(const char *optname) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optGetDouble: Optionfile handle not initialized.");
		return 0.;
	}
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
    	snprintf(sval, 255, "*** Internal Error. Option %s is not a double (it is %d)", optname, dataType);
    	PrintOut(AllMask, sval);
      return 0.;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return dval;
  }

 	snprintf(sval, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, sval);
	return 0.;
}

char* GamsModel::optGetString(const char *optname, char *sval) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optGetString: Optionfile handle not initialized.");
		return sval;
	}
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataString) {
    	snprintf(oname, 255, "*** Internal Error. Option %s is not a string (it is %d)", optname, dataType);
 	  	PrintOut(AllMask, oname);
   	  return sval;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return sval;
  }

 	snprintf(oname, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, oname);
	return sval;
}

void GamsModel::optSetInteger(const char *optname, int ival) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optSetInteger: Optionfile handle not initialized.");
		return;
	}
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not an integer (it is %d)\n", optname, dataType);
			PrintOut(AllMask, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, ival, 0.0, "");
    
    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					PrintOut(AllMask, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}
		
		return;
  }
  
 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, stmp);
}

void GamsModel::optSetDouble(const char *optname, double dval) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optSetDouble: Optionfile handle not initialized.");
		return;
	}
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not a double (it is %d)\n", optname, dataType);
			PrintOut(AllMask, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, 0, dval, "");
    
    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					PrintOut(AllMask, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}
		
		return;
  }
  
 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, stmp);
}

void GamsModel::optSetString(const char *optname, char *sval) {
	if (NULL==optionshandle) {
		PrintOut(AllMask, "GamsModel::optSetString: Optionfile handle not initialized.");
		return;
	}
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataString) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not a string (it is %d)\n", optname, dataType);
			PrintOut(AllMask, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, 0, 0.0, sval);

    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					PrintOut(AllMask, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}
		
		return;
  }
  
 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s", optname);
	PrintOut(AllMask, stmp);
}
