// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSHANDLERSMAG_HPP_
#define GAMSHANDLERSMAG_HPP_

#include "GamsHandler.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

struct smagRec;

/** Forwards requests for output and point transformations to the GAMS I/O library smag.
 */
class GamsHandlerSmag : public GamsHandler {
private:
	smagRec* smag;
	
	static inline int translateMask(const PrintMask& mask);
	
public:
	GamsHandlerSmag(smagRec* smag_ = NULL) : smag(smag_) { }
	
	~GamsHandlerSmag() { }
	
	void setSmag(smagRec* smag_) { smag=smag_; }
	
	void print(PrintMask mask, const char* msg) const;
	
	void println(PrintMask mask, const char* msg) const;
	
	void flush(PrintMask mask=AllMask) const;

	void translateToGamsSpaceX(const double* x_, double objval_, double* x) const;
	void translateToGamsSpaceLB(const double* lb_, double* lb) const;
	void translateToGamsSpaceUB(const double* ub_, double* ub) const;
	void translateFromGamsSpaceX(const double* x_, double* x) const;
	bool translateFromGamsSpaceCol(const int* indices_, int* indices, int nr) const;
	int translateToGamsSpaceCol(int colindex) const;
	int translateToGamsSpaceRow(int rowindex) const;

	double getMInfinity() const;
	double getPInfinity() const;

	int getObjSense() const;
	
	int getColCountGams() const;
	int getObjVariable() const;
	int getObjRow() const;

	const char* getSystemDir() const;

	bool isDictionaryWritten() const;
	const char* dictionaryFile() const;
	int dictionaryVersion() const;
}; // class GamsHandlerSmag


#endif /*GAMSHANDLERSMAG_HPP_*/
