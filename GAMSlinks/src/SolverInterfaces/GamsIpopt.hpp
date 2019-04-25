// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_HPP_
#define GAMSIPOPT_HPP_

#include "GamsSolver.hpp"

#include "IpTNLP.hpp"
#include "IpIpoptApplication.hpp"

/** GAMS interface to Ipopt */
class GamsIpopt : public GamsSolver
{
private:
   struct gmoRec*         gmo;                /**< GAMS modeling object */
   struct gevRec*         gev;                /**< GAMS environment */

   bool ipoptlicensed;
   Ipopt::SmartPtr<Ipopt::IpoptApplication> ipopt;

public:
   GamsIpopt()
   : gmo(NULL),
     gev(NULL),
     ipoptlicensed(false)
   { }

   int readyAPI(
      struct gmoRec*     gmo_,
      struct optRec*     opt
   );

   int callSolver();

   /** indicates whether the solver interface and solver supports the modifyProblem call */
   static int haveModifyProblem()
   {
      return -1;
   }
};

#endif