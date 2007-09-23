// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

#ifndef GamsMessageHandler_H
#define GamsMessageHandler_H

#include "GamsModel.hpp"
#include "CoinMessageHandler.hpp"

/** A COIN-OR message handler that writes into the GAMS status- and logfile.
 */
class GamsMessageHandler : public CoinMessageHandler {
public:

	/** Constructor.
	 * @param GMptr The GamsModel required for printing.
	 */  
  GamsMessageHandler(GamsModel* GMptr);

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

private:
  GamsModel *GMptr_;
  int rmlblanks_;
};

#endif // GamsMessageHandler_H
