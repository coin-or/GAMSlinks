// Copyright (C) GAMS Development and others 2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSCBCHEURBBTRACE_HPP_
#define GAMSCBCHEURBBTRACE_HPP_

#include "CbcHeuristic.hpp"
#include "CbcModel.hpp"
#include "GamsBBTrace.h"
#include "CoinTime.hpp"

/** CbcHeuristic that reports bounds to a GAMS branch-and-bound trace object */
class GamsCbcHeurBBTrace : public CbcHeuristic
{
private:
   GAMS_BBTRACE*         bbtrace;            /**< GAMS bbtrace data structure */

public:
   GamsCbcHeurBBTrace(
      GAMS_BBTRACE*      bbtrace_            /**< GAMS bbtrace data structure */
   )
   : bbtrace(bbtrace_)
   { }

   CbcHeuristic* clone() const
   {
      return new GamsCbcHeurBBTrace(bbtrace);
   }

   void resetModel(
      CbcModel*          model
   )
   { }

   int solution(
      double&            objectiveValue,
      double*            newSolution
   )
   {
      double primalbnd;

      if( model_->getSolutionCount() > 0 )
         primalbnd = model_->getObjValue();
      else
         primalbnd = model_->getObjSense() * model_->getInfinity();

      GAMSbbtraceAddLine(bbtrace, model_->getNodeCount(), CoinGetTimeOfDay(), model_->getBestPossibleObjValue(), primalbnd);

      return 0;
   }

   using CbcHeuristic::solution;
};

#endif // GAMSCBCHEURBBTRACE_HPP_
