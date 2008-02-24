// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsHandlerSmag.hpp"

// smag.h will try to include stdio.h and stdarg.h
// so we include cstdio and cstdarg before if we know that we have them
#ifdef HAVE_CSTDIO
#include <cstdio>
#endif
#ifdef HAVE_CSTDARG
#include <cstdarg>
#endif
#include "smag.h"

#include "CoinHelperFunctions.hpp"

inline int GamsHandlerSmag::translateMask(const PrintMask& mask) {
	switch (mask) {
		case LogMask: return SMAG_LOGMASK;
		case StatusMask : return SMAG_STATUSMASK;
		default: return SMAG_ALLMASK;
	}
}

void GamsHandlerSmag::print(PrintMask mask, const char* msg) const {
	smagStdOutputPrintX(smag, translateMask(mask), msg, 0);
}
	
void GamsHandlerSmag::println(PrintMask mask, const char* msg) const {
	smagStdOutputPrintX(smag, translateMask(mask), msg, 1);
}
	
void GamsHandlerSmag::flush(PrintMask mask /*=AllMask*/) const {
	smagStdOutputFlush(smag, translateMask(mask));
}

void GamsHandlerSmag::translateToGamsSpaceX(const double* x_, double objval_, double* x) const {
	// If fixed variables have been removed by SMAG, then we should initialize x so that the values of these variables get into there.
	if (smag->squeezeFixedVars)
		CoinDisjointCopyN(smag->gms.gxlb, smag->gms.ncols, x);

	// Also the objective variable will probably have been removed.
  x[smag->gObjCol]=objval_;
	
  // and now the regular variables
	int N=smagColCount(smag);
	int *colmap=smag->colMapS2G;
  for (int j=0; j<N; ++j, ++colmap)
  	x[*colmap]=x_[j];
}

void GamsHandlerSmag::translateFromGamsSpaceX(const double* x_, double* x) const {
	int N=smag->gms.ncols;
	int *colmap=smag->colMapG2S;
	for (int j=0; j<N; ++j, ++colmap)
		if (*colmap>=0)
			x[*colmap]=x_[j];
}


void GamsHandlerSmag::translateToGamsSpaceLB(const double* lb_, double* lb) const {
	if (smag->squeezeFixedVars)
		CoinDisjointCopyN(smag->gms.gxlb, smag->gms.ncols, lb);
  lb[smag->gObjCol]=-smagGetInf(smag);
	
	int N=smagColCount(smag);
	int *colmap=smag->colMapS2G;
  for (int j=0; j<N; ++j, ++colmap)
  	lb[*colmap]=lb_[j];
}

void GamsHandlerSmag::translateToGamsSpaceUB(const double* ub_, double* ub) const {
	if (smag->squeezeFixedVars)
		CoinDisjointCopyN(smag->gms.gxub, smag->gms.ncols, ub);
  ub[smag->gObjCol]=smagGetInf(smag);

	int N=smagColCount(smag);
	int *colmap=smag->colMapS2G;
  for (int j=0; j<N; ++j, ++colmap)
  	ub[*colmap]=ub_[j];
}

bool GamsHandlerSmag::translateFromGamsSpaceCol(const int* indices_, int* indices, int nr) const {
	int* colmap=smag->colMapG2S;
	const int* ind_=indices_;
	int* ind =indices;
	for (int j=nr; j; --j, ++ind_, ++ind) {
		if (*ind_<0 || *ind_>=smag->gms.ncols) return false; // index out of bounds
		if (colmap[*ind_]<0) return false;
		*ind=colmap[*ind_];
	}
	return true;
}

int GamsHandlerSmag::translateToGamsSpaceCol(int colindex) const {
	if (colindex<0 || colindex>=smagColCount(smag)) return -1; // index out of bounds
	return smag->colMapS2G[colindex];
}

int GamsHandlerSmag::translateToGamsSpaceRow(int rowindex) const {
	if (rowindex<0 || rowindex>=smagRowCount(smag)) return -1; // index out of bounds
	return smag->rowMapS2G[rowindex];
}

double GamsHandlerSmag::getMInfinity() const {
	return -smagGetInf(smag);	
}

double GamsHandlerSmag::getPInfinity() const {
	return smagGetInf(smag);
}

int GamsHandlerSmag::getObjSense() const {
	return smag->minim;
}

int GamsHandlerSmag::getColCountGams() const {
	return smag->gms.ncols;
}

int GamsHandlerSmag::getObjVariable() const {
	return smag->gObjCol;
}

const char* GamsHandlerSmag::getSystemDir() const {
	return smag->gms.sysDir;
}

bool GamsHandlerSmag::isDictionaryWritten() const {
	// TODO: have not found this in smag
	// using this as an approximation
	return smag->gms.dictFileName!=NULL; 
}

const char* GamsHandlerSmag::dictionaryFile() const {
	return smag->gms.dictFileName;
}

int GamsHandlerSmag::dictionaryVersion() const {
	return smag->gms.dictFileType;
}
