// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSMESSAGEHANDLER_HPP_
#define GAMSMESSAGEHANDLER_HPP_

#include "GAMSlinksConfig.h"
#include "CoinMessageHandler.hpp"

struct gmoRec;

/** A CoinUtils message handler that writes using the routines of a Gams Modeling Object.
 */
class GamsMessageHandler : public CoinMessageHandler {
private:
	struct gmoRec* gmo;
public:
	/** Constructor.
	 * @param gmo_ A Gams Modeling Object to access the GAMS status- and logfile.
	 */  
  GamsMessageHandler(struct gmoRec* gmo_);

  /** Destructor.
   */
//  ~GamsMessageHandler();

  /** Sets the detail level of the current message.
   * @param detail Detail level.
   */
  void setCurrentDetail(int detail);

  /** Returns the detail level of the current message.
   * @return Detail level.
   */
  int getCurrentDetail() const;

	/** Prints the message from the message buffer.
	 * Removes all newlines at the end of the message buffer.
	 * If currentMessage().detail() is smaller then 2, the message is written to logfile and statusfile, otherwise it is written only to the logfile.
	 * @return Zero. 
	 */
  int print();
  
  /** Creates a copy of this message handler.
   */
  CoinMessageHandler* clone() const { return new GamsMessageHandler(gmo); }
};


#endif /*GAMSMESSAGEHANDLER_HPP_*/
