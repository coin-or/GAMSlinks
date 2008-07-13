// Copyright (C) GAMS Development 2006-2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

#ifndef GamsMessageHandler_H
#define GamsMessageHandler_H

#include "GAMSlinksConfig.h"

#include "CoinPragma.hpp"
#include "CoinMessageHandler.hpp"

#include "GamsHandler.hpp"

#ifdef CBC_THREAD
#include <pthread.h>
#endif

/** A COIN-OR message handler that writes into the GAMS status- and logfile.
 */
class GamsMessageHandler : public CoinMessageHandler {
public:

	/** Constructor.
	 * @param gams_ A GAMS handler to access the GAMS status- and logfile.
	 */  
  GamsMessageHandler(GamsHandler& gams_);

  ~GamsMessageHandler();

	/** Sets the number of spaces to remove at the front of a message.
	 */
  inline void setRemoveLBlanks(int rm) { rmlblanks_ = rm; }
  
  /** Sets the detail level of the current message.
   */
  void setCurrentDetail(int detail);

  /** Returns the detail level of the current message.
   */
  int getCurrentDetail() const;

	/** Prints the message from the message buffer.
	 * Removes at most rmlblanks_ from the beginning and all newlines at the end of the message buffer.
	 * If currentMessage().detail() is smaller then 2, the message is written to logfile and statusfile, otherwise it is written only to the logfile.
	 * If the pointer to the GamsModel is not set, the output goes to standard out. 
	 */  
  int print();
  
  CoinMessageHandler* clone() const { return new GamsMessageHandler(gams); }

private:
	GamsHandler& gams;
  int rmlblanks_;
#ifdef CBC_THREAD
  pthread_mutex_t print_mutex;
#endif
};

#endif // GamsMessageHandler_H
