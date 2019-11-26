// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsMessageHandler.hpp"

#include <cstdio>
#include <cstring>

#include "gevmcc.h"

int GamsMessageHandler::print()
{
   const char* messageOut;

   /* ensure we are exclusively calling gevLog... */
   (void) pthread_mutex_lock(&mutex);

   messageOut = messageBuffer();
   if( messageOut[strlen(messageOut)-1] == '\n' )
      if( currentMessage_.detail() < 2 )
         gevLogStatPChar(gev, messageOut);
      else
         gevLogPChar(gev, messageOut);
   else
      if( currentMessage_.detail() < 2 )
         gevLogStat(gev, messageOut);
      else
         gevLog(gev, messageOut);

   (void) pthread_mutex_unlock(&mutex);

   return 0;
}
