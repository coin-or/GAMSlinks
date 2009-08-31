// Copyright (C) GAMS Development and others 2009
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

#ifndef HAVE_SNPRINTF
#ifdef HAVE__SNPRINTF
#define snprintf _snprintf
#else
#error "Do not have snprintf of _snprintf."
#endif
#endif

#include "gdxcc.h"
#include "gmomcc.h"
#include "gevmcc.h"

GamsGDX::GamsGDX(struct gmoRec* gmo_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL), gdx(NULL)
{ }

GamsGDX::~GamsGDX() {
	if (gdx) {
		gdxFree(&gdx);
		gdxLibraryUnload(); //TODO what if someone else uses GDX?
	}
}

void GamsGDX::reportError(int n) const {
	char message[256];
	if (!gdxErrorStr(gdx, n, message))
		gevLogStatPChar(gev, "Retrieve of GDX I/O error message failed.");
	else
		gevLogStatPChar(gev, message);
}

bool GamsGDX::init() {
	char errormsg[128];
	if (!gdxCreate(&gdx, errormsg, 128)) {
		gevLogStatPChar(gev, "Initialization of GDX I/O library failed: ");
		gevLogStat(gev, errormsg);
		return false;
	}

	gdxSVals_t sVals;
	gdxGetSpecialValues(gdx, sVals);
	sVals[GMS_SVIDX_MINF] = gmoMinf(gmo);
  sVals[GMS_SVIDX_PINF] = gmoPinf(gmo);
  gdxSetSpecialValues(gdx, sVals);

	return true;
}

bool GamsGDX::writePoint(const double* x, const double* rc, double objval, const char* filename) const {
	if (!gdx) {
		gevLogStatPChar(gev, "Error: GDX library not initialized.\n");
		return false;
	}
	if (!gmoDict(gmo)) {
		gevLogStatPChar(gev, "Error: Do not have dictionary.\n");
		return false;
	}
	if (!x) {
		gevLogStatPChar(gev, "Error: No primal values given.\n");
		return false;
	}

	int errornr;
	gdxOpenWrite(gdx, filename, "GamsGDX", &errornr);
	if (errornr) { reportError(errornr); return false; }

	gdxUelIndex_t uelIndices;
  int symIndex, symDim, oldSym=-1;
  char symName[GMS_UEL_IDENT_SIZE];
  char quote;
  gdxStrIndex_t Indices;
  gdxStrIndexPtrs_t IndicesPtr;
  gdxValues_t Values;
  GDXSTRINDEXPTRS_INIT(Indices, IndicesPtr);
  int colnr;

	for (int i = -1; i < gmoN(gmo); ++i) { // -1 stands for objective variable
		colnr = i >= 0 ? gmoGetjModel(gmo, i) : gmoObjVar(gmo);
  	if (0==dctColUels(dict.dict, colnr, &symIndex, uelIndices, &symDim)) {
  		gevLogStatPChar(gev, "Error in dctColUels.\n");
  		return false;
  	}
	  if (dctSymName(dict.dict, symIndex, symName, GMS_UEL_IDENT_SIZE)) {
	  	gevLogStatPChar(gev, "Error in dctSymName.\n");
	  	return false;
	  }

		if (oldSym!=symIndex) { // we start a new symbol
			if (oldSym!=-1) { // close old symbol, if we are not at the first; test i==0 should do the same
				if (!gdxDataWriteDone(gdx)) {
					char msg[256];
					snprintf(msg, 256, "Error in gdxDataWriteDone for symbol nr. %d\n", oldSym);
					gevLogStatPChar(gev, msg);
					return false;
				}
			}
			if (!gdxDataWriteStrStart(gdx, symName, NULL, symDim, dt_var, 0)) {
				gevLogStatPChar(gev, "Error in gdxDataWriteStrStart for symbol ");
				gevLogStat(gev, symName);
				return false;
			}
			oldSym=symIndex;
		}

		for (int k = 0;  k < symDim;  k++) {
    	if (dctUelLabel(dict.dict, uelIndices[k], &quote, IndicesPtr[k], GMS_UEL_IDENT_SIZE)) {
    		gevLogStatPChar(gev, "Error in dctUelLabel.\n");
				return false;
			}
		}

		Values[GMS_VAL_SCALE] = 1.0;
		if (i >= 0) {
			Values[GMS_VAL_LEVEL] = x[i];
			Values[GMS_VAL_MARGINAL] = rc ? rc[i] : 0.;
			Values[GMS_VAL_LOWER] = gmoGetVarLowerOne(gmo, i);
			Values[GMS_VAL_UPPER] = gmoGetVarUpperOne(gmo, i);
		} else {
			Values[GMS_VAL_LEVEL] = objval;
			Values[GMS_VAL_MARGINAL] = 0.;
			Values[GMS_VAL_LOWER] = gmoMinf(gmo);
			Values[GMS_VAL_UPPER] = gmoPinf(gmo);
		}

		if (!gdxDataWriteStr(gdx, const_cast<const char**>(IndicesPtr), Values)) {
			gevLogStatPChar(gev, "Error in gdxDataWriteStr for symbol ");
			gevLogStat(gev, symName);
			return false;
		}
  }

	if (!gdxDataWriteDone(gdx)) { // finish last symbol
		char msg[256];
		snprintf(msg, 256, "Error in gdxDataWriteDone for symbol nr. %d\n", oldSym);
		gevLogStatPChar(gev, msg);
		return false;
	}

  if ((errornr = gdxClose(gdx))) { reportError(errornr); return false; }

	return true;
}
