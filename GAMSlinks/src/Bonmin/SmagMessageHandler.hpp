// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#ifndef SmagMessageHandler_H
#define SmagMessageHandler_H

#include "GAMSlinksConfig.h"
#include "CoinMessageHandler.hpp"

// smag.h will try to include stdio.h and stdarg.h
// so we include cstdio and cstdarg before if we know that we have them
#ifdef HAVE_CSTDIO
#include <cstdio>
#endif
#ifdef HAVE_CSTDARG
#include <cstdarg>
#endif
#include "smag.h"

/** A COIN-OR message handler that writes using the SMAG routines.
 */
class SmagMessageHandler : public CoinMessageHandler {
public:

	/** Constructor.
	 * @param smag_ Handle for the smag interface.
	 */  
  SmagMessageHandler(smagHandle_t smag_);

	/** Prints the message from the message buffer.
	 * If currentMessage().detail() is smaller then 2, the message is written to logfile and statusfile, otherwise it is written only to the logfile.
	 */  
  int print();
  
  CoinMessageHandler* clone() const { return new SmagMessageHandler(smag); }

private:
	smagHandle_t smag;
};

#endif // SmagMessageHandler_H
