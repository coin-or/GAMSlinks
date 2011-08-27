// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsJournal.hpp"
#include "GAMSlinksConfig.h"

#include "gevmcc.h"

#include <cstdio>
#include <cstdarg>
#include <cassert>

#ifndef HAVE_VSNPRINTF
#ifdef HAVE__VSNPRINTF
#define vsnprintf _vsnprintf
#else
#define NOVSNPRINTF
#endif
#endif

#ifdef NOVSNPRINTF
#define VSNPRINTF fakevsnprintf
static char hugeBufVSN[10020];
static int fakevsnprintf(
   char*                 str,
   size_t                size,
   const char*           format,
   va_list               ap
)
{
   /* this better not overflow! */
   int rc = vsprintf(hugeBufVSN, format, ap);
   assert(rc >= 0);
   assert(rc < sizeof(hugeBufVSN));
   int n = rc;
   if( n < size-1 )
      n = size-1;
   if( n >= 0 )
   {
      strncpy(str, hugeBufVSN, n);
      str[n] = '\0';
   }

   return rc;
}
#else
#define VSNPRINTF vsnprintf
#endif

void GamsJournal::PrintImpl(
   Ipopt::EJournalCategory category,
   Ipopt::EJournalLevel    level,
   const char*             str
)
{
   assert(gev != NULL);

   if( level <= status_level )
      gevLogStatPChar(gev, str);
   else
      gevLogPChar(gev, str);
}

void GamsJournal::PrintfImpl(
   Ipopt::EJournalCategory category,
   Ipopt::EJournalLevel    level,
   const char*             pformat,
   va_list                 ap
)
{
   static char outBuf[10000];
#ifdef HAVE_VA_COPY
   va_list apcopy;
   va_copy(apcopy, ap);
   VSNPRINTF(outBuf, sizeof(outBuf), pformat, apcopy);
   va_end(apcopy);
#else
   VSNPRINTF(outBuf, sizeof(outBuf), pformat, ap);
#endif

   PrintImpl(category, level, outBuf);
}
