// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsMessageHandler.hpp"

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

#include "gevmcc.h"

GamsMessageHandler::GamsMessageHandler(gevHandle_t gev_)
: gev(gev_)
{ }

void GamsMessageHandler::setCurrentDetail(int detail) {
	currentMessage_.setDetail(detail);
}

int GamsMessageHandler::getCurrentDetail() const {
	return currentMessage_.detail();
}

// Print message, return 0 normally
int GamsMessageHandler::print() {
	const char *messageOut = messageBuffer();

  if (messageOut[strlen(messageOut)-1] == '\n')
  	if (currentMessage_.detail() < 2)
  		gevLogStatPChar(gev, messageOut);
  	else
  		gevLogPChar(gev, messageOut);
  else
  	if (currentMessage_.detail() < 2)
  		gevLogStat(gev, messageOut);
  	else
  		gevLog(gev, messageOut);

	return 0;
}
