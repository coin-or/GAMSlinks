// Copyright (C) GAMS Development and others 2008-2009
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

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
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

#include "gmomcc.h"
#include "dctmcc.h"

GamsDictionary::GamsDictionary(gmoHandle_t gmo_, dctHandle_t dict_)
: gmo(gmo_), dict(dict_), have_dictread(dict_ ? true : false), dict_is_own(false)
{ }

GamsDictionary::~GamsDictionary() {
	if (dict_is_own) {
		// close dictionary
		// if (have_dictread) dctClose(dict);
		// free dictionary handler
//TODO		if (dict) gcdFree(&dict);
		// unload dictdll library
		dctLibraryUnload(); //TODO what if there are several dictionaries open?
	}
}

void GamsDictionary::setGCD(struct dctRec* gcd_) {
  char msg[256];
	assert(!dict);
        assert(0==dctGetReady(msg, sizeof(msg)));
	dict = gcd_;
	dict_is_own = false;
	have_dictread = true; // TODO there seem to be no function to get this info from gcd, so we assume a dictionary has been read
}

bool GamsDictionary::initGCD() {
	assert(gmo);
	// get the Dictionary File Handling set up
	char buffer[512];
	if (!dctCreate(&dict, buffer, sizeof(buffer))) {
		gmoLogStatPChar(gmo, "\n*** Could not load dictionary handler: ");
		gmoLogStat(gmo, buffer);
		return false;
	}
	dict_is_own = true;
	return true;
}

bool GamsDictionary::readDictionary() {
	if (!dict && !initGCD()) return false; // error setting up dictionary file handling
	if (have_dictread) return true; // dictionary open already
	if (!gmoDictionary(gmo)) return false; // do not have dictionary file

	char buffer[512];
	char dictfilename[1024];
	gmoNameDict(gmo, dictfilename);

	if (dctLoadEx(dict, dictfilename, gmoDictionary(gmo), buffer, sizeof(buffer))) {
		gmoLogStatPChar(gmo, "Error reading dictionary file: ");
		gmoLogStat(gmo, buffer);
		return false;
	}

	have_dictread = true;
	return true;
}

char* GamsDictionary::getRowName(int rownr, char *buffer, int bufLen) {
	rownr = gmoGetiModel(gmo, rownr);
	return getModelRowName(rownr, buffer, bufLen);
}

char* GamsDictionary::getModelRowName(int rownr, char *buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;

  if (rownr<0 || rownr >= dctNRows(dict)) return NULL;
  if (dctRowUels(dict, rownr, &symIndex, uelIndices, &symDim)) return NULL;

  return constructName(buffer, bufLen, symIndex, uelIndices, symDim);
}

char* GamsDictionary::getColName(int colnr, char *buffer, int bufLen) {
	colnr = gmoGetjModel(gmo, colnr);
	return getModelColName(colnr, buffer, bufLen);
}

char* GamsDictionary::getModelColName(int colnr, char *buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;

  if (colnr < 0 || colnr >= dctNCols(dict)) return NULL;
  if (dctColUels(dict, colnr, &symIndex, uelIndices, &symDim)) return NULL;

  return constructName(buffer, bufLen, symIndex, uelIndices, symDim);
}

char* GamsDictionary::getObjName(char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;

	int objrow = gmoObjRow(gmo);

  if (objrow<0 || objrow >= dctNRows(dict)) return NULL;
  if (dctRowUels(dict, objrow, &symIndex, uelIndices, &symDim)) return NULL;

  return constructName(buffer, bufLen, symIndex, uelIndices, symDim);
}

char* GamsDictionary::getColText(int colnr, char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;
	char quote;

  colnr = gmoGetjModel(gmo, colnr);

  if (colnr < 0 || colnr >= dctNCols(dict)) return NULL;
  if (dctColUels(dict, colnr, &symIndex, uelIndices, &symDim)) return NULL;

	if (dctSymText(dict, symIndex, &quote, buffer, bufLen)) return NULL;

	return buffer;
}

char* GamsDictionary::getRowText(int rownr, char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;
	char quote;

  rownr = gmoGetiModel(gmo, rownr);

  if (rownr<0 || rownr >= dctNRows(dict)) return NULL;
  if (dctRowUels(dict, rownr, &symIndex, uelIndices, &symDim)) return NULL;

	if (dctSymText(dict, symIndex, &quote, buffer, bufLen)) return NULL;

	return buffer;
}

char* GamsDictionary::getObjText(char* buffer, int bufLen) {
	if (!have_dictread) return NULL;
	gdxUelIndex_t uelIndices;
	int symIndex, symDim;
	char quote;

	int objrow = gmoObjRow(gmo);

  if (objrow<0 || objrow >= dctNRows(dict)) return NULL;
  if (dctRowUels(dict, objrow, &symIndex, uelIndices, &symDim)) return NULL;

	if (dctSymText(dict, symIndex, &quote, buffer, bufLen)) return NULL;

	return buffer;
}

char* GamsDictionary::constructName(char* buffer, int bufLen, int symIndex, int* uelIndices, int symDim) {
	if (bufLen <= 0)
		return NULL;
  char symName[GMS_UEL_IDENT_SIZE];
  int pos = 0;
  int len;

  if (dctSymName(dict, symIndex, symName, GMS_UEL_IDENT_SIZE))
  	return NULL;

  len = strlen(symName);

  strncpy(buffer, symName, len < bufLen ? bufLen : len);
  if (len >= bufLen || 0 == symDim) {
  	buffer[bufLen-1] = 0;
  	return buffer;
  }

  pos = len;

  char quote;
  gdxStrIndex_t strIndex;
  gdxStrIndexPtrs_t strIndexPtrs;
  GDXSTRINDEXPTRS_INIT(strIndex, strIndexPtrs);

  buffer[pos++] = '(';
  for (int k=0;  k<symDim && pos < bufLen;  k++) {
    if (dctUelLabel(dict, uelIndices[k], &quote, strIndexPtrs[k], GMS_UEL_IDENT_SIZE))
    	return NULL;

    if (' ' != quote) buffer[pos++] = quote;

    len = strlen(strIndexPtrs[k]);
    if (pos+len >= bufLen-1) {
    	buffer[pos++] = '*';
    	break;
    }

    strncpy(buffer+pos, strIndexPtrs[k], len);
    pos += len;
    if (' ' != quote) {
    	buffer[pos++] = quote;
	    if (pos >= bufLen) break;
    }
    buffer[pos++] = (k+1==symDim) ? ')' : ',';
  }
  buffer[pos >= bufLen ? bufLen-1 : pos] = '\0';

  return buffer;
}
