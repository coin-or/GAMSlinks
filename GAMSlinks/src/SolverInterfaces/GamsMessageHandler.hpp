// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSMESSAGEHANDLER_HPP_
#define GAMSMESSAGEHANDLER_HPP_

#include "CoinMessageHandler.hpp"

struct gevRec;

/** message handler that writes via the Gams Environment */
class GamsMessageHandler : public CoinMessageHandler
{
protected:
   struct gevRec*        gev;                /**< GAMS environment */

public:
   GamsMessageHandler(
      struct gevRec*     gev_                /**< GAMS environment */
   )
   : gev(gev_)
   { }

   /** sets detail level of current message */
   void setCurrentDetail(
      int                detail              /**< new detail level */
   )
   {
      currentMessage_.setDetail(detail);
   }

   /** gives detail level of current message */
   int getCurrentDetail() const
   {
      return currentMessage_.detail();
   }

   /** prints message from the message buffer
    * Removes all newlines at the end of the message buffer.
    * If currentMessage().detail() is smaller then 2, the message is written to logfile and statusfile, otherwise it is written only to the logfile.
    */
   int print();

   /** creates a copy of this message handler */
   CoinMessageHandler* clone() const
   {
      return new GamsMessageHandler(gev);
   }
};

#endif /*GAMSMESSAGEHANDLER_HPP_*/
