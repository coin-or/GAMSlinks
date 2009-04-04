// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSJOURNAL_HPP_
#define GAMSJOURNAL_HPP_

#include "GAMSlinksConfig.h"

#include "IpJournalist.hpp"

struct gmoRec;

/** A particular Journal implementation that uses the GMO routines for output.
 */
class GamsJournal : public Ipopt::Journal {
private:
	struct gmoRec* gmo;
	/** highest level for output to status file */
	Ipopt::EJournalLevel status_level;

public:
  GamsJournal(struct gmoRec* gmo_, const char* name, Ipopt::EJournalLevel default_level, Ipopt::EJournalLevel status_level_ = Ipopt::J_SUMMARY)
  : Ipopt::Journal(name, default_level), gmo(gmo_), status_level(status_level_)
  { }

  ~GamsJournal() { }

protected:
  void PrintImpl(Ipopt::EJournalCategory category, Ipopt::EJournalLevel level, const char* str);

  void PrintfImpl(Ipopt::EJournalCategory category, Ipopt::EJournalLevel level, const char* pformat, va_list ap);

  void FlushBufferImpl() { }
};

#endif /*GAMSJOURNAL_HPP_*/
