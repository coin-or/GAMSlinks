// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsHandlerIOLib.hpp"

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

#include "CoinHelperFunctions.hpp"

void GamsHandlerIOLib::print(PrintMask mask, const char* msg) const {
	if (mask&GamsHandler::LogMask && 0 != gfiolog) {
    fprintf (gfiolog, msg); fflush (gfiolog);
  }
  if (mask&GamsHandler::StatusMask && 0 != gfiosta) 
    fprintf (gfiosta, msg);
}
	
void GamsHandlerIOLib::println(PrintMask mask, const char* msg) const {
  if (mask&GamsHandler::LogMask && 0 != gfiolog) {
    fprintf (gfiolog, "%s\n", msg); fflush (gfiolog);
  }
  if (mask&GamsHandler::StatusMask && 0 != gfiosta) 
    fprintf (gfiosta, "%s\n", msg);
}
	
void GamsHandlerIOLib::flush(PrintMask mask /*=AllMask*/) const {
  if (mask&GamsHandler::LogMask && 0 != gfiolog)
    fflush (gfiolog);
  if (mask&GamsHandler::StatusMask && 0 != gfiosta) 
    fflush (gfiosta);	
}



void GamsHandlerIOLib::translateToGamsSpaceX(const double* x_, double objval_, double* x) const {
	if (is_reformulated) {
		CoinDisjointCopyN(x_, iolib.iobvar, x);
		CoinDisjointCopy(x_+iolib.iobvar, x_+(iolib.ncols-1), x+iolib.iobvar+1);
		x[iolib.iobvar]=objval_;
	} else {
		CoinDisjointCopyN(x_, iolib.ncols, x);
	}
}

void GamsHandlerIOLib::translateFromGamsSpaceX(const double* x_, double* x) const {
	if (is_reformulated) {
		CoinDisjointCopyN(x_, iolib.iobvar, x);
		CoinDisjointCopy(x_+iolib.iobvar+1, x_+iolib.ncols, x+iolib.iobvar);
	} else {
		CoinDisjointCopyN(x_, iolib.ncols, x);
	}
}


void GamsHandlerIOLib::translateToGamsSpaceLB(const double* lb_, double* lb) const {
	if (is_reformulated) {
		CoinDisjointCopyN(lb_, iolib.iobvar, lb);
		CoinDisjointCopy(lb_+iolib.iobvar, lb_+(iolib.ncols-1), lb+iolib.iobvar+1);
		lb[iolib.iobvar]=iolib.usrminf;
	} else {
		CoinDisjointCopyN(lb_, iolib.ncols, lb);
	}	
}

void GamsHandlerIOLib::translateToGamsSpaceUB(const double* ub_, double* ub) const {
	if (is_reformulated) {
		CoinDisjointCopyN(ub_, iolib.iobvar, ub);
		CoinDisjointCopy(ub_+iolib.iobvar, ub_+(iolib.ncols-1), ub+iolib.iobvar+1);
		ub[iolib.iobvar]=iolib.usrpinf;
	} else {
		CoinDisjointCopyN(ub_, iolib.ncols, ub);
	}	
}

bool GamsHandlerIOLib::translateFromGamsSpaceCol(const int* indices_, int* indices, int nr) const {
  if (!is_reformulated) {
  	if (indices!=indices_) CoinDisjointCopyN(indices_, nr, indices);
  	return true; // nothing to do
  }

	for (int i=0; i<nr; ++i) {
		if (indices_[i]==iolib.iobvar)
			return false;
		indices[i]=(indices_[i]>iolib.iobvar ? indices_[i]-1 : indices_[i]); // decrement indices of variables behind objective variable 
	}

	return true;
}

int GamsHandlerIOLib::translateToGamsSpaceCol(int colindex) const {
	if (is_reformulated && colindex>=iolib.iobvar)
		++colindex;
	if (colindex<0 || colindex>=iolib.ncols)
		return -1;
	return colindex;
}

int GamsHandlerIOLib::translateToGamsSpaceRow(int rowindex) const {
	if (is_reformulated && rowindex>=iolib.slplro-1)
		++rowindex;
	if (rowindex<0 || rowindex>=iolib.nrows)
		return -1;
	return rowindex;
}


double GamsHandlerIOLib::getMInfinity() const {
	return iolib.usrminf;
}

double GamsHandlerIOLib::getPInfinity() const {
	return iolib.usrpinf;
}

int GamsHandlerIOLib::getObjSense() const {
	return (0 == iolib.idir) ? 1 : -1;
}

int GamsHandlerIOLib::getColCount() const {
	return iolib.ncols - (is_reformulated ? 1 : 0);
}

int GamsHandlerIOLib::getColCountGams() const {
	return iolib.ncols;
}

int GamsHandlerIOLib::getObjVariable() const {
	return iolib.iobvar;
}
int GamsHandlerIOLib::getObjRow() const {
	return is_reformulated ? iolib.slplro-1 : -1;
}

const char* GamsHandlerIOLib::getSystemDir() const {
	return iolib.gsysdr;
}

bool GamsHandlerIOLib::isDictionaryWritten() const {
	return iolib.dictFileWritten;
}

const char* GamsHandlerIOLib::dictionaryFile() const {
	return iolib.flndic;
}

int GamsHandlerIOLib::dictionaryVersion() const {
	return iolib.dictVersion;
}

