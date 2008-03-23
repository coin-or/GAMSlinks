// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske


#include "GamsDictionary.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#include "dict.h"

GamsDictionary::GamsDictionary(GamsHandler& gams_)
: gams(gams_), dict(NULL)
{ }

GamsDictionary::~GamsDictionary() {
	// Close dictionary
	if (dict) gcdFree(dict);
}

bool GamsDictionary::readDictionary() {
	if (dict) gcdFree(dict); // close directory, if some is already open
	
	if (!gams.isDictionaryWritten()) return false;
	if (gcdLoad(&dict, gams.dictionaryFile(), gams.dictionaryVersion())) {
		gams.print(GamsHandler::AllMask, "Error reading dictionary file.\n");
		dict=NULL;
		return false;
	}	
	
	return true;
}

char* GamsDictionary::getRowName(int rownr, char *buffer, int bufLen) {
	if (!dict) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

  int grownr=gams.translateToGamsSpaceRow(rownr);

  if (grownr<0 || grownr >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, grownr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices); 
}

char* GamsDictionary::getColName(int colnr, char *buffer, int bufLen) {
	if (!dict) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;
  
  int gcolnr=gams.translateToGamsSpaceCol(colnr);

  if (gcolnr < 0 || gcolnr >= gcdNCols(dict)) return NULL;
  if (gcdColUels(dict, gcolnr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices);
}

char* GamsDictionary::getObjName(char* buffer, int bufLen) {
	if (!dict) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

	int gobjrow=gams.getObjRow();
	
  if (gobjrow<0 || gobjrow >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, gobjrow, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices); 
}

char* GamsDictionary::getColText(int colnr, char* buffer, int bufLen) {
	if (!dict) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;
  
  int gcolnr=gams.translateToGamsSpaceCol(colnr);

  if (gcolnr < 0 || gcolnr >= gcdNCols(dict)) return NULL;
  if (gcdColUels(dict, gcolnr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
	if (gcdSymText(dict, lSym, buffer, bufLen, &quote) == NULL) return NULL;
	
	return buffer;
}

char* GamsDictionary::getRowText(int rownr, char* buffer, int bufLen) {
	if (!dict) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;

  int grownr=gams.translateToGamsSpaceRow(rownr);

  if (grownr<0 || grownr >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, grownr, &lSym, uelIndices, &nIndices) != 0) return NULL;

	if (gcdSymText(dict, lSym, buffer, bufLen, &quote) == NULL) return NULL;

	return buffer;
}

char* GamsDictionary::getObjText(char* buffer, int bufLen) {
	if (!dict) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;

	int gobjrow=gams.getObjRow();

  if (gobjrow<0 || gobjrow >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, gobjrow, &lSym, uelIndices, &nIndices) != 0) return NULL;

	if (gcdSymText(dict, lSym, buffer, bufLen, &quote) == NULL) return NULL;

	return buffer;
}

char* GamsDictionary::constructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices) {
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
