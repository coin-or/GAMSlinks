// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOptions.hpp"

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

#include "gmomcc.h"
#include "gevmcc.h"
#include "optcc.h"

GamsOptions::GamsOptions(gmoHandle_t gmo_, optHandle_t opt_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL), optionshandle(opt_), opt_is_own(false)
{ }

GamsOptions::~GamsOptions() {
	if (opt_is_own) {
		if (optionshandle) optFree(&optionshandle);
		optLibraryUnload(); // TODO what if someone else uses the options library?
	}
}

bool GamsOptions::initOpt(const char* solvername) {
	/* Get the Option File Handling set up */
	char buffer[512];
	if (!optCreate(&optionshandle, buffer, 512)) {
		gevLogStatPChar(gev, "\n*** Could not create optionfile handle: ");
		gevLogStat(gev, buffer);
		return false;
	}

	gevGetStrOpt(gev, gevNameSysDir, buffer);
	buffer[511] = '\0';
	int len = strlen(buffer);
	if (snprintf(buffer+len, 512-len, "opt%s.def", solvername) >= 512) {
		gevLogStatPChar(gev, "\n*** Path to GAMS system directory too long.\n");
		return false;
	}

	if (optReadDefinition(optionshandle, buffer)) {
		int itype;
		for (int i = 1; i <= optMessageCount(optionshandle); ++i) {
			optGetMessage(optionshandle, i, buffer, &itype);
			gevLogStat(gev, buffer);
		}
		optClearMessages(optionshandle);
		return false;
	}
	optEOLOnlySet(optionshandle, 1);

	opt_is_own = true;
	return true;
}

void GamsOptions::setGMO(struct gmoRec* gmo_) {
	gmo = gmo_;
	gev = gmo_ ?  (gevRec*)gmoEnvironment(gmo_) : NULL;
}

void GamsOptions::setOpt(struct optRec* opt_) {
	assert(!optionshandle);

	char buffer[512];
	if (!optGetReady(buffer, 512)) {
		gevLogStatPChar(gev, "\n*** Could not load optionfile library: ");
		gevLogStat(gev, buffer);
		return;
	}

	opt_is_own = false;
	optionshandle = opt_;
}

bool GamsOptions::readOptionsFile(const char* solvername, const char* optfilename) {
	if (!optionshandle && !initOpt(solvername)) return false; // initializing option file handling failed
	if (optfilename == NULL) return true;
	/* Read option file */
  optEchoSet(optionshandle, 1);
  optReadParameterFile(optionshandle, optfilename);
  if (optMessageCount(optionshandle)) {
	  char buffer[255];
	  int itype;
	  for (int i = 1; i <= optMessageCount(optionshandle); ++i) {
  	  optGetMessage(optionshandle, i, buffer, &itype);
    	if (itype <= optMsgFileLeave || itype == optMsgUserError)
    		gevLogStat(gev, buffer);
	  }
	  optClearMessages(optionshandle);
		optEchoSet(optionshandle, 0);
	  return false;
	} else {
		optEchoSet(optionshandle, 0);
		return true;
	}
}

bool GamsOptions::isKnown(const char* optname) {
	assert(optionshandle != NULL);
	int i, refNum;
  return optFindStr(optionshandle, optname, &i, &refNum);
}

bool GamsOptions::isDefined(const char *optname) {
	assert(optionshandle != NULL);
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType;

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    return isDefined;
  } else {
  	char buffer[255];
  	snprintf(buffer, 255, "*** Internal Error. Unknown option %s\n", optname);
		gevLogStatPChar(gev, buffer);
    return false;
  }
}

int GamsOptions::getInteger(const char *optname) {
	assert(optionshandle != NULL);
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
    	snprintf(sval, 255, "*** Internal Error. Option %s is not an integer (it is %d)\n", optname, dataType);
    	gevLogStatPChar(gev, sval);
      return 0;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return ival;
  }

 	snprintf(sval, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, sval);
	return 0;
}

double GamsOptions::getDouble(const char *optname) {
	assert(optionshandle!=NULL);
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
    	snprintf(sval, 255, "*** Internal Error. Option %s is not a double (it is %d)\n", optname, dataType);
    	gevLogStatPChar(gev, sval);
      return 0.;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return dval;
  }

 	snprintf(sval, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, sval);
	return 0.;
}

char* GamsOptions::getString(const char *optname, char *sval) {
	assert(optionshandle!=NULL);
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char oname[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataString) {
    	snprintf(oname, 255, "*** Internal Error. Option %s is not a string (it is %d)\n", optname, dataType);
    	gevLogStatPChar(gev, oname);
   	  return sval;
    }
    optGetValuesNr(optionshandle, i, oname, &ival, &dval, sval);
    return sval;
  }

 	snprintf(oname, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, oname);
	return sval;
}

void GamsOptions::setInteger(const char *optname, int ival) {
	assert(optionshandle!=NULL);
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not an integer (it is %d)\n", optname, dataType);
			gevLogStatPChar(gev, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, ival, 0.0, "");

    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					gevLogStatPChar(gev, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}

		return;
  }

 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, stmp);
}

void GamsOptions::setDouble(const char *optname, double dval) {
	assert(optionshandle!=NULL);
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not a double (it is %d)\n", optname, dataType);
			gevLogStatPChar(gev, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, 0, dval, "");

    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					gevLogStatPChar(gev, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}

		return;
  }

 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, stmp);
}

void GamsOptions::setString(const char *optname, const char *sval) {
	assert(optionshandle!=NULL);
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(optionshandle, optname, &i, &refNum)) {
    optGetInfoNr(optionshandle, i, &isDefined, &isDefinedRecent, &refNum, &dataType, &optType, &subType);
    if (dataType != optDataString) {
			snprintf(stmp, 255, "*** Internal Error. Option %s is not a string (it is %d)\n", optname, dataType);
			gevLogStatPChar(gev, stmp);
      return;
    }
    optSetValuesNr(optionshandle, i, 0, 0.0, sval);

    if (optMessageCount(optionshandle)) {
    	char buffer[255];
	    for (i=1; i<=optMessageCount(optionshandle); i++) {
	      optGetMessage(optionshandle, i, stmp, &k);
	      if (k==optMsgUserError || k<optMsgFileEnter) {
					snprintf(buffer, 255, "%d: %s\n", k, stmp);
					gevLogStatPChar(gev, buffer);
	      }
			}
	    optClearMessages(optionshandle);
		}

		return;
  }

 	snprintf(stmp, 255, "*** Internal Error. Unknown option %s\n", optname);
	gevLogStatPChar(gev, stmp);
}
