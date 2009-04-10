// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsJournal.hpp"

#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
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

#ifdef HAVE_CSTDARG
#include <cstdarg>
#else
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#error "don't have header file for stdarg"
#endif
#endif

#ifndef HAVE_VSNPRINTF
#ifdef HAVE__VSNPRINTF
#define vsnprintf _vsnprintf
#else
#define NOVSNPRINTF
#endif
#endif

#if defined(NOVSNPRINTF)
# define VSNPRINTF fakevsnprintf
static char        hugeBufVSN[10020];
static int
fakevsnprintf(char *str, size_t size, const char *format, va_list ap) {
  /* this better not overflow! */
	int rc = vsprintf (hugeBufVSN, format, ap);
	int n = rc;
	if (n<size-1) n=size-1;
  if (n >= 0) {
    strncpy (str, hugeBufVSN, n);
    str[n] = '\0';
  }
  return rc;
} /* fakevsnprintf */
#else
# define VSNPRINTF vsnprintf
#endif

void GamsJournal::PrintImpl(Ipopt::EJournalCategory category, Ipopt::EJournalLevel level, const char* str) {
	if (level <= status_level)
		gmoLogStatPChar(gmo, str);
	else
		gmoLogPChar(gmo, str);
}

void GamsJournal::PrintfImpl(Ipopt::EJournalCategory category, Ipopt::EJournalLevel level, const char* pformat, va_list ap) {
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
