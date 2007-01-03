// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "GamsMessageHandler.hpp"
#include <cstdio>
#include <cstring>

#define LOGMASK    0x1
#define STATUSMASK 0x2
#define ALLMASK    LOGMASK|STATUSMASK

// Print message, return 0 normally
int GamsMessageHandler::print() {
  const char *messageOut = messageBuffer();
  char *lastchar;
  int i=rmlblanks_;

  // white space at the beginning
  while (i-- > 0 && *messageOut == ' ') 
      messageOut++;
  lastchar = (char*) messageOut + strlen(messageOut); lastchar--;
  while (*lastchar == '\n') {
    *lastchar = 0; lastchar--;
  }
  if (0 == GMptr_)
    printf("%s", messageOut);
  else {
    int detail = currentMessage().detail();
    if (detail < 2)
      GMptr_->PrintOut(ALLMASK, messageOut);
    else
      GMptr_->PrintOut(STATUSMASK, messageOut);
  }
  return 0;
}

GamsMessageHandler::GamsMessageHandler()
: GMptr_(NULL), rmlblanks_(1) 
{ }

void
GamsMessageHandler::setGamsModel(GamsModel *GMptr)
{ 
  GMptr_= GMptr; 
}


