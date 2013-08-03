// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors:  Stefan Vigerske

#ifndef GAMSOS_HPP_
#define GAMSOS_HPP_

#include "GamsSolver.hpp"

#include <string>

class GamsOptions;
class OSInstance;
class OSResult;

/** GAMS interface to Optimization Services (OS) */
class GamsOS : public GamsSolver
{
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct optRec*        opt;                /**< GAMS options object */

   bool remoteSolve(
      OSInstance*        osinstance,         /**< Optimization Services instance */
      std::string&       osol,               /**< Optimization Services options string in OSoL format */
      GamsOptions&       gamsopt             /**< GAMS options object */
   );

   bool processResult(
      std::string*       osrl,               /**< optimization result as string in OSrL format */
      OSResult*          osresult,           /**< optimization result as object */
      GamsOptions&       gamsopt             /**< GAMS options object */
   );

   bool contriveSOS(
      std::string&       osil                /**< OSiL string */
   );

public:
   GamsOS()
   : gmo(NULL),
     gev(NULL),
     opt(NULL)
   { }

   ~GamsOS()
   { }

   int readyAPI(
      struct gmoRec*     gmo,                /**< GAMS modeling object */
      struct optRec*     opt                 /**< GAMS options object */
   );

   int callSolver();

   /** indicates whether the solver interface and solver supports the modifyProblem call */
   static int haveModifyProblem()
   {
      return -1;
   }
};

#endif /*GAMSOS_HPP_*/
