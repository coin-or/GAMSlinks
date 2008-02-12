// Copyright (C) GAMS Development 2006-2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

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

GamsMessageHandler::GamsMessageHandler(GamsHandler& gams_)
: gams(gams_), rmlblanks_(1) 
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
  int i=rmlblanks_;

  // white space at the beginning
  while (i-- > 0 && *messageOut == ' ') 
  	++messageOut;

//TODO: change back to distinguishing between allmask and logmask as soon as smag bug fixed
  if (messageOut[strlen(messageOut)-1]=='\n')
    gams.print(GamsHandler::LogMask, messageOut);
  else
    gams.println(GamsHandler::LogMask, messageOut);
//  	gams.print(currentMessage_.detail() < 2 ? GamsHandler::AllMask : GamsHandler::LogMask, messageOut);
//  else
//  	gams.println(currentMessage_.detail() < 2 ? GamsHandler::AllMask : GamsHandler::LogMask, messageOut);

  return 0;
}
