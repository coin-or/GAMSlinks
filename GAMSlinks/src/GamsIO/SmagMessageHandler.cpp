// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#include "SmagMessageHandler.hpp"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

SmagMessageHandler::SmagMessageHandler(smagHandle_t smag_)
: smag(smag_) 
{ }

// Print message, return 0 normally
int SmagMessageHandler::print() {
  const char *messageOut = messageBuffer();
  // white space at the beginning
  if (*messageOut == ' ') ++messageOut;

  if (0 == smag)
    printf("%s\n", messageOut);
  else {
  	int smag_mask = currentMessage().detail() < 2 ? SMAG_ALLMASK : SMAG_LOGMASK; 
  	smagStdOutputPrintX(smag, smag_mask, messageOut, 0);
  	smagStdOutputPrintX(smag, smag_mask, "\n", 0); // and put a newline
  }
  return 0;
}

