// Copyright (C) 2006-2008 GAMS Development and others 
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

GamsMessageHandler::GamsMessageHandler(GamsHandler& gams_)
: gams(gams_), rmlblanks_(1) 
{ 
#ifdef CBC_THREAD
	pthread_mutex_init(&print_mutex, NULL);
#endif
}

GamsMessageHandler::~GamsMessageHandler() {
#ifdef CBC_THREAD
	pthread_mutex_destroy(&print_mutex);
#endif
}

void GamsMessageHandler::setCurrentDetail(int detail) {
	currentMessage_.setDetail(detail);
}

int GamsMessageHandler::getCurrentDetail() const {
	return currentMessage_.detail();
}

// Print message, return 0 normally
int GamsMessageHandler::print() {
#ifdef CBC_THREAD
	pthread_mutex_lock(&print_mutex);
#endif
	const char *messageOut = messageBuffer();
  int i=rmlblanks_;

  // white space at the beginning
  while (i-- > 0 && *messageOut == ' ') 
  	++messageOut;

  if (messageOut[strlen(messageOut)-1]=='\n')
  	gams.print(currentMessage_.detail() < 2 ? GamsHandler::AllMask : GamsHandler::LogMask, messageOut);
  else
  	gams.println(currentMessage_.detail() < 2 ? GamsHandler::AllMask : GamsHandler::LogMask, messageOut);

#ifdef CBC_THREAD
	pthread_mutex_unlock(&print_mutex);
#endif
	return 0;
}
