// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: SmagJournal.hpp 298 2008-01-04 14:03:20Z stefan $
//
// Author: Stefan Vigerske
// from the IpFileJournal class in IPOPT

#ifndef __SMAGJOURNAL_HPP__
#define __SMAGJOURNAL_HPP__

#include "GAMSlinksConfig.h"

// smag.h will try to include stdio.h and stdarg.h
// so we include cstdio and cstdarg before if we know that we have them
#ifdef HAVE_CSTDIO
#include <cstdio>
#endif
#ifdef HAVE_CSTDARG
#include <cstdarg>
#endif
#include "smag.h"

#include "IpJournalist.hpp"

using namespace Ipopt;

/** A particular Journal implementation that uses the SMAG routines for output.
 */
class SmagJournal : public Journal {
public:
  /** Constructor.
   * @param smag_ A pointer to the SMAG structure.
   * @param name The name of this journal.
   * @param default_level The default print level for this journal.
   * @param status_level_ Maximum level where we still write to status file.
   */
  SmagJournal(smagHandle_t smag_, const char* name, EJournalLevel default_level, EJournalLevel status_level_=J_SUMMARY)
  : Journal(name, default_level), smag(smag_), status_level(status_level_)
  { }

  /** Destructor.
   */
  ~SmagJournal() { }

protected:
  virtual void PrintImpl(EJournalCategory category, EJournalLevel level, const char* str) {
  	smagStdOutputPrintX(smag, level<=status_level ? SMAG_ALLMASK : SMAG_LOGMASK, str, 0);
  }

  virtual void PrintfImpl(EJournalCategory category, EJournalLevel level, const char* pformat, va_list ap);

  virtual void FlushBufferImpl() {
  	smagStdOutputFlush(smag, SMAG_ALLMASK);
	}

private:
	smagHandle_t smag;
	EJournalLevel status_level;
	
  /**@name Default Compiler Generated Methods
   * (Hidden to avoid implicit creation/calling).
   * These methods are not implemented and 
   * we do not want the compiler to implement
   * them for us, so we declare them private
   * and do not define them. This ensures that
   * they will not be implicitly created/called. */
  SmagJournal();
  SmagJournal(const SmagJournal&);
  void operator=(const SmagJournal&);
};

#endif // __SMAGJOURNAL_HPP__
