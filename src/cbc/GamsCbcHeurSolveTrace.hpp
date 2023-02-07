// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSCBCHEURSOLVETRACE_HPP_
#define GAMSCBCHEURSOLVETRACE_HPP_

#include "CbcHeuristic.hpp"
#include "CbcModel.hpp"
#include "GamsSolveTrace.h"
#include "CoinTime.hpp"

/** CbcHeuristic that reports bounds to a GAMS solve trace object */
class GamsCbcHeurSolveTrace : public CbcHeuristic
{
private:
   GAMS_SOLVETRACE*      solvetrace;         /**< GAMS solve trace data structure */
   double                objfactor;          /**< multiplier for objective function values */

public:
   GamsCbcHeurSolveTrace(
      GAMS_SOLVETRACE*   solvetrace_,        /**< GAMS solve trace data structure */
      double             objfactor_ = 1.0    /**< multiplier for objective function values */
   )
   : solvetrace(solvetrace_),
     objfactor(objfactor_)
   { }

   CbcHeuristic* clone() const
   {
      return new GamsCbcHeurSolveTrace(solvetrace, objfactor);
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
      GAMSsolvetraceAddLine(solvetrace, model_->getNodeCount(),
         objfactor * model_->getBestPossibleObjValue(),
         model_->getSolutionCount() > 0 ? objfactor * model_->getObjValue() : (objfactor > 0.0 ? 1.0 : -1.0) * model_->getObjSense() * model_->getInfinity());

      return 0;
   }

   using CbcHeuristic::solution;
};

#endif // GAMSCBCHEURSOLVETRACE_HPP_
