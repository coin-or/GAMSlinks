// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske
// from the IpFileJournal class in IPOPT

#ifndef __SMAGJOURNAL_HPP__
#define __SMAGJOURNAL_HPP__

#include "GAMSlinksConfig.h"
#include "smag.h"
#include "IpJournalist.hpp"

using namespace Ipopt;

/** SmagJournal class. This is a particular Journal implementation that
 *  uses the SMAG routines for output.
 */
class SmagJournal : public Journal {
public:
  /** Constructor. */
  SmagJournal(smagHandle_t smag_, unsigned int smag_mask_, const char* name, EJournalLevel default_level)
  : Journal(name, default_level), smag(smag_), smag_mask(smag_mask_)
  { }

  /** Destructor. */
  ~SmagJournal() { }

protected:
  /**@name Implementation version of Print methods - Overloaded from Journal base class.
   */
  //@{
  /** Print to the designated output location */
  virtual void PrintImpl(const char* str);

  /** Printf to the designated output location */
  virtual void PrintfImpl(const char* pformat, va_list ap);

  /** Flush output buffer.*/
  virtual void FlushBufferImpl() {
  	smagStdOutputFlush(smag, SMAG_ALLMASK);
	}
  //@}

private:
	smagHandle_t smag;
	unsigned int smag_mask;
  /**@name Default Compiler Generated Methods
   * (Hidden to avoid implicit creation/calling).
   * These methods are not implemented and 
   * we do not want the compiler to implement
   * them for us, so we declare them private
   * and do not define them. This ensures that
   * they will not be implicitly created/called. */
  //@{
  /** Default Constructor */
  SmagJournal();

  /** Copy Constructor */
  SmagJournal(const SmagJournal&);

  /** Overloaded Equals Operator */
  void operator=(const SmagJournal&);
  //@}
};

#endif // __SMAGJOURNAL_HPP__
