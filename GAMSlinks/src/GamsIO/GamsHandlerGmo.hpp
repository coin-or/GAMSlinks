// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsHandlerGmo.hpp 571 2008-09-28 14:20:17Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSHANDLERGMO_HPP_
#define GAMSHANDLERGMO_HPP_

#include "GAMSlinksConfig.h"
#include "GamsHandler.hpp"

extern "C" {
#include "gmocc.h"
}

/** Forwards requests for output, point transformations, dictionary files, and other things to GMO.
 */
class GamsHandlerGmo : public GamsHandler {
private:
	gmoHandle_t gmo;
	
	mutable char* sysdir;
	mutable char* dictfile;

public:
	GamsHandlerGmo(gmoHandle_t gmo_);
	~GamsHandlerGmo();

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
	
	int getColCount() const;
	int getColCountGams() const;
	int getObjVariable() const;
	int getObjRow() const;
	
	const char* getSystemDir() const;
	
	bool isDictionaryWritten() const;
	const char* dictionaryFile() const;
	int dictionaryVersion() const;
};


#endif /*GAMSHANDLERGMO_HPP_*/
