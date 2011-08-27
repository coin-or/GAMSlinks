// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSJOURNAL_HPP_
#define GAMSJOURNAL_HPP_

#include "IpJournalist.hpp"

struct gevRec;

/** a particular Ipopt Journal implementation that uses the GMO routines for output */
class GamsJournal : public Ipopt::Journal
{
private:
   struct gevRec* gev;

   /** highest level for output to status file */
   Ipopt::EJournalLevel status_level;

public:
   GamsJournal(
      struct gevRec*       gev_,             /**< GAMS environment */
      const char*          name,             /**< journalist name */
      Ipopt::EJournalLevel default_level,    /**< default journal level */
      Ipopt::EJournalLevel status_level_ = Ipopt::J_SUMMARY /**< journal level up to which print into status file */
   )
   : Ipopt::Journal(name, default_level),
     gev(gev_),
     status_level(status_level_)
   { }

protected:
   void PrintImpl(
      Ipopt::EJournalCategory category,
      Ipopt::EJournalLevel    level,
      const char*             str
   );

   void PrintfImpl(
      Ipopt::EJournalCategory category,
      Ipopt::EJournalLevel    level,
      const char*             pformat,
      va_list                 ap
   );

   void FlushBufferImpl()
   { }
};

#endif /*GAMSJOURNAL_HPP_*/
