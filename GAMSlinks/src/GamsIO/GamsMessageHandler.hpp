// Copyright (C) GAMS Development 2006-2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsMessageHandler.hpp 510 2008-08-16 19:31:27Z stefan $
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

/** A CoinUtils message handler that writes into the GAMS status- and logfile.
 * Thread safe if CBC_THREAD is defined.
 */
class GamsMessageHandler : public CoinMessageHandler {
public:
	/** Constructor.
	 * @param gams_ A GAMS handler to access the GAMS status- and logfile.
	 */  
  GamsMessageHandler(GamsHandler& gams_);

  /** Destructor.
   */
  ~GamsMessageHandler();

	/** Sets the number of spaces to remove at the front of a message.
	 */
  inline void setRemoveLBlanks(int rm) { rmlblanks_ = rm; }
  
  /** Sets the detail level of the current message.
   * @param detail Detail level.
   */
  void setCurrentDetail(int detail);

  /** Returns the detail level of the current message.
   * @return Detail level.
   */
  int getCurrentDetail() const;

	/** Prints the message from the message buffer.
	 * Removes at most rmlblanks_ from the beginning and all newlines at the end of the message buffer.
	 * If currentMessage().detail() is smaller then 2, the message is written to logfile and statusfile, otherwise it is written only to the logfile.
	 * If the pointer to the GamsModel is not set, the output goes to standard out.
	 * @return Zero. 
	 */
  int print();
  
  /** Creates a copy of this message handler.
   */
  CoinMessageHandler* clone() const { return new GamsMessageHandler(gams); }

private:
	GamsHandler& gams;
  int rmlblanks_;
#ifdef CBC_THREAD
  pthread_mutex_t print_mutex;
#endif
};

#endif // GamsMessageHandler_H
