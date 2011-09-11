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

public:
   GamsCbcHeurBBTrace(
      GAMS_BBTRACE*      bbtrace_,           /**< GAMS bbtrace data structure */
      double             objfactor_ = 1.0    /**< multiplier for objective function values */
   )
   : bbtrace(bbtrace_),
     objfactor(objfactor_)
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
      GAMSbbtraceAddLine(bbtrace, model_->getNodeCount(),
         objfactor * model_->getBestPossibleObjValue(),
         model_->getSolutionCount() > 0 ? objfactor * model_->getObjValue() : (objfactor > 0.0 ? 1.0 : -1.0) * model_->getObjSense() * model_->getInfinity());

      return 0;
   }

   using CbcHeuristic::solution;
};

#endif // GAMSCBCHEURBBTRACE_HPP_
