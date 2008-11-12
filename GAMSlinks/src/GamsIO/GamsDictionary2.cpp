// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske


#include "GamsDictionary2.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

extern "C" {
#include "gcdcc.h"
}

GamsDictionary::GamsDictionary(GamsHandler& gams_)
: gams(gams_), dict(NULL), have_dictread(false)
{
	/* Get the Dictionary File Handling set up */
	char buffer[512];
	if (!gcdCreate(&dict, buffer, sizeof(buffer))) {
		gams.print(GamsHandler::AllMask, "\n*** Could not load dictionary handler: "); 
		gams.println(GamsHandler::AllMask, buffer); 
		exit(EXIT_FAILURE);
	}

//	if (snprintf(buffer, 512, "%sopt%s.def", gams.getSystemDir(), solvername)>=512) {
//		gams.print(GamsHandler::AllMask, "\n*** Path to GAMS system directory too long.\n");
//		exit(EXIT_FAILURE);
//	}
	
}

GamsDictionary::~GamsDictionary() {
	// close dictionary
	if (have_dictread) gcdClose(dict);
	// free dictionary handler
	if (dict) gcdFree(&dict);
	// unload dictdll library
	gcdLibraryUnload();
}

bool GamsDictionary::readDictionary() {
	if (have_dictread) return true; // have dictionary open already
	if (!gams.isDictionaryWritten()) return false; // do not have dictionary file
	
	char buffer[512];
	if (gcdLoadEx(dict, gams.dictionaryFile(), buffer, sizeof(buffer))) {
		gams.print(GamsHandler::AllMask, "Error reading dictionary file: ");
		gams.println(GamsHandler::AllMask, buffer);
		return false;
	}
	
	have_dictread = true;
	return true;
}

char* GamsDictionary::getRowName(int rownr, char *buffer, int bufLen) {
	if (!have_dictread) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

  int grownr = gams.translateToGamsSpaceRow(rownr);

  if (grownr<0 || grownr >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, grownr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices); 
}

char* GamsDictionary::getColName(int colnr, char *buffer, int bufLen) {
	if (!have_dictread) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;
  
  int gcolnr = gams.translateToGamsSpaceCol(colnr);

  if (gcolnr < 0 || gcolnr >= gcdNCols(dict)) return NULL;
  if (gcdColUels(dict, gcolnr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices);
}

char* GamsDictionary::getObjName(char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
  int
    uelIndices[10],
    nIndices,
    lSym;

	int gobjrow = gams.getObjRow();
	
  if (gobjrow<0 || gobjrow >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, gobjrow, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
  return constructName(buffer, bufLen, lSym, uelIndices, nIndices); 
}

char* GamsDictionary::getColText(int colnr, char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;
  
  int gcolnr = gams.translateToGamsSpaceCol(colnr);

  if (gcolnr < 0 || gcolnr >= gcdNCols(dict)) return NULL;
  if (gcdColUels(dict, gcolnr, &lSym, uelIndices, &nIndices) != 0) return NULL;
  
	if (gcdSymText(dict, lSym, &quote, buffer, bufLen) == NULL) return NULL;
	
	return buffer;
}

char* GamsDictionary::getRowText(int rownr, char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;

  int grownr = gams.translateToGamsSpaceRow(rownr);

  if (grownr<0 || grownr >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, grownr, &lSym, uelIndices, &nIndices) != 0) return NULL;

	if (gcdSymText(dict, lSym, &quote, buffer, bufLen) == NULL) return NULL;

	return buffer;
}

char* GamsDictionary::getObjText(char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	char
		quote;
  int
    uelIndices[10],
    nIndices,
    lSym;

	int gobjrow = gams.getObjRow();

  if (gobjrow<0 || gobjrow >= gcdNRows(dict)) return NULL;
  if (gcdRowUels(dict, gobjrow, &lSym, uelIndices, &nIndices) != 0) return NULL;

	if (gcdSymText(dict, lSym, &quote, buffer, bufLen) == NULL) return NULL;

	return buffer;
}

char* GamsDictionary::constructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices) {
	if (bufLen <= 0)
		return NULL;
  char
    quote,
    *targ,
    *end,
    *s,
    tbuf[32];
	
  if ((s = gcdSymName (dict, lSym, tbuf, sizeof(tbuf))) == NULL) return NULL;

  targ = buffer;
  end = targ + bufLen-1; // keep one byte for a closing \0

  while (targ < end && *s != '\0') *targ++ = *s++;

  if (0 == nIndices || targ==end) {
    *targ = '\0';
    return buffer;
  }

  *targ++ = '(';
  for (int k=0;  k<nIndices && targ < end;  k++) {
    s = gcdUelLabel (dict, uelIndices[k], &quote, tbuf, sizeof(tbuf));
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
