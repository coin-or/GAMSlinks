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
   double                objfactor;          /**< multiplier for objective function values */
   double                starttime;          /**< time when started */

public:
   GamsCbcHeurBBTrace(
      GAMS_BBTRACE*      bbtrace_,           /**< GAMS bbtrace data structure */
      double             objfactor_ = 1.0    /**< multiplier for objective function values */
   )
   : bbtrace(bbtrace_),
     objfactor(objfactor_),
     starttime(CoinGetTimeOfDay())
   { }

   CbcHeuristic* clone() const
   {
      return new GamsCbcHeurBBTrace(bbtrace, objfactor);
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
      double dualbnd;

      dualbnd = objfactor * model_->getBestPossibleObjValue();

      if( model_->getSolutionCount() > 0 )
         primalbnd = objfactor * model_->getObjValue();
      else
         primalbnd = (objfactor > 0.0 ? 1.0 : -1.0) * model_->getObjSense() * model_->getInfinity();

      GAMSbbtraceAddLine(bbtrace, model_->getNodeCount(), CoinGetTimeOfDay() - starttime, dualbnd, primalbnd);

      return 0;
   }

   using CbcHeuristic::solution;
};

#endif // GAMSCBCHEURBBTRACE_HPP_
