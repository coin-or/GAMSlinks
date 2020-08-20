// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsMessageHandler.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>

#include "gevmcc.h"

int GamsMessageHandler::print()
{
   const char* messageOut;

   /* ensure we are exclusively calling gevLog... */
   assert(mutex);
   mutex->lock();

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

   mutex->unlock();

   return 0;
}
