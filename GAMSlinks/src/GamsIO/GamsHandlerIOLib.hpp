// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSHANDLERIOLIB_HPP_
#define GAMSHANDLERIOLIB_HPP_

#include "GamsHandler.hpp"

/** Forwards requests for output and point transformations to the GAMS I/O library iolib.
 */
class GamsHandlerIOLib : public GamsHandler {
private:
	bool is_reformulated;
	
public:
	/** Constructor.
	 * @param is_reformulated_ Whether we should assume that the objective row had been moved into the objective function and the objective variable been eliminated.
	 */
	GamsHandlerIOLib(bool is_reformulated_) : is_reformulated(is_reformulated_) { }
	
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


#endif /*GAMSHANDLERIOLIB_HPP_*/
