// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsGDX.hpp"

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

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

extern "C" {
#include "gdxcc.h"
#include "dict.h"
}

GamsGDX::GamsGDX(GamsHandler& gams_, GamsDictionary& dict_)
: gams(gams_), dict(dict_), gdx(NULL)
{ }
	
GamsGDX::~GamsGDX() {
	if (gdx) {
		gdxFree(&gdx);
		gdxLibraryUnload();
	}
}
	
void GamsGDX::reportError(int n) const {
	char message[256];
	if (!gdxErrorStr(gdx, n, message)) {
		gams.print(GamsHandler::AllMask, "Retrieve of GDX I/O error message failed.");
	} else {
		gams.print(GamsHandler::AllMask, message);
	}
}

bool GamsGDX::init() {
	char errormsg[128];
	if (!gdxCreate(&gdx, errormsg, 128)) {
		gams.print(GamsHandler::AllMask, "Initialization of GDX I/O library failed: ");
		gams.println(GamsHandler::AllMask, errormsg);
		return false;
	}
	
	gdxSVals_t sVals;
	gdxGetSpecialValues(gdx, sVals);
	sVals[GMS_SVIDX_MINF] = gams.getMInfinity();
  sVals[GMS_SVIDX_PINF] = gams.getPInfinity();
  gdxSetSpecialValues(gdx, sVals);

	return true;
}

bool GamsGDX::writePoint(const double* x, const double* rc, double objval, const char* filename) const {
	if (!gdx) {
		gams.print(GamsHandler::AllMask, "Error: GDX library not initialized.\n");
		return false;
	}
	if (!dict.haveNames()) {
		gams.print(GamsHandler::AllMask, "Error: Dictionary does not have names.\n");
		return false;
	}
	
	if (!x) {
		gams.print(GamsHandler::AllMask, "Error: No primal values given.\n");
		return false;
	}
	
	double* x_ = new double[gams.getColCountGams()];
	gams.translateToGamsSpaceX(x, objval, x_);
	
	double* rc_ = NULL;
	if (rc) {
		rc_ = new double[gams.getColCountGams()];
		gams.translateToGamsSpaceX(rc, 0., rc_);		
	}
	
	int errornr;
	gdxOpenWrite(gdx, filename, "GamsGDX", &errornr);
	if (errornr) { reportError(errornr); return false;	}

  int uelIndices[CIO_MAX_INDEX_DIM], symIndex, symDim, oldSym=-1;
  char symName[CIO_UEL_IDENT_SIZE], indexName[CIO_UEL_IDENT_SIZE], quote;
  gdxStrIndex_t Indices;
  gdxStrIndexPtrs_t IndicesPtr;
  gdxValues_t Values;
  GDXSTRINDEXPTRS_INIT(Indices, IndicesPtr);
  
	for (int i=0; i<gams.getColCountGams(); ++i) {
  	if (gcdColUels(dict.dict, i, &symIndex, uelIndices, &symDim)) {
			gams.print(GamsHandler::AllMask, "Error in gcdColUels.\n");
  		return false;
  	}
	  if (!gcdSymName(dict.dict, symIndex, symName, CIO_UEL_IDENT_SIZE)) {
			gams.print(GamsHandler::AllMask, "Error in gcdSymName.\n");
	  	return false;
	  }

		if (oldSym!=symIndex) { // we start a new symbol
			if (oldSym!=-1) { // close old symbol, if we are not at the first; test i==0 should do the same
				if (!gdxDataWriteDone(gdx)) {
					char msg[256];
					snprintf(msg, 256, "Error in gdxDataWriteDone for symbol nr. %d\n", oldSym);
					gams.print(GamsHandler::AllMask, msg);
					return false;
				}
			}
			if (!gdxDataWriteStrStart(gdx, symName, NULL, symDim, dt_var, 0)) {
				gams.print(GamsHandler::AllMask, "Error in gdxDataWriteStrStart for symbol ");
				gams.println(GamsHandler::AllMask, symName);
				return false;
			}
			oldSym=symIndex;
		}

		for (int k = 0;  k < symDim;  k++) {
    	if (!gcdUelLabel(dict.dict, uelIndices[k], IndicesPtr[k], CIO_UEL_IDENT_SIZE, &quote)) {
				gams.print(GamsHandler::AllMask, "Error in gcdUelLabel.\n");
				return false;
			}
		}
		
		memset(Values, 0, sizeof(gdxValues_t));
		
    Values[GMS_VAL_SCALE] = 1.0;
    Values[GMS_VAL_LEVEL] = x_[i];
    if (rc_) Values[GMS_VAL_MARGINAL] = rc_[i];
//    if (NULL != lb) Values[GMS_VAL_LOWER   ] = lb[i];
//    if (NULL != ub) Values[GMS_VAL_UPPER   ] = ub[i];

		if (!gdxDataWriteStr(gdx, (const char**)IndicesPtr, Values)) {
			gams.print(GamsHandler::AllMask, "Error in gdxDataWriteStr for symbol ");
			gams.println(GamsHandler::AllMask, symName);
			return false;
		}
  }

	if (!gdxDataWriteDone(gdx)) { // finish last symbol
		char msg[256];
		snprintf(msg, 256, "Error in gdxDataWriteDone for symbol nr. %d\n", oldSym);
		gams.print(GamsHandler::AllMask, msg);
		return false;
	}

  if ((errornr=gdxClose(gdx))) { reportError(errornr); return false; }

	delete[] x_;
	delete[] rc_;
	
	return true;
}
