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

struct gevRec;

/** A CoinUtils message handler that writes using the routines of a Gams Modeling Object.
 */
class GamsMessageHandler : public CoinMessageHandler {
private:
	struct gevRec* gev;
public:
	/** Constructor.
	 * @param gev_ A Gams Environment Object to access the GAMS status- and logfile.
	 */
  GamsMessageHandler(struct gevRec* gev_);

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
  CoinMessageHandler* clone() const { return new GamsMessageHandler(gev); }
};


#endif /*GAMSMESSAGEHANDLER_HPP_*/
