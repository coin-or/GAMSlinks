// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSOSI_HPP_
#define GAMSOSI_HPP_

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct optRec* optHandle_t;

#include <sstream>

class GamsMessageHandler;
class GamsOutputStreamBuf;
class OsiSolverInterface;

/** GAMS interface to solvers with Osi interface */
class GamsOsi
{
public:
   typedef enum { CPLEX, GUROBI, MOSEK, XPRESS } OSISOLVER;

private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct optRec*        opt;                /**< GAMS options object */

   GamsMessageHandler*   msghandler;         /**< message handler */
   OsiSolverInterface*   osi;                /**< solver interface */
   OSISOLVER             solverid;           /**< ID of used solver */

   /** loads problem from GMO into OSI */
   bool setupProblem();

   /** sets up warm start information (initial solution or basis) */
   bool setupStartingPoint();

   /** sets up solver parameters */
   bool setupParameters();

   /** sets up solver callbacks */
   bool setupCallbacks();

   /** clears solver callbacks, if needed */
   bool clearCallbacks();

   /** writes solution from OSI into GMO */
   bool writeSolution(
      double             cputime,            /**< CPU time spend by solver */
      double             walltime,           /**< wallclock time spend by solver */
      bool               cpxsubproberr,      /**< has an error in solving a CPLEX subproblem occured? */
      bool&              solwritten          /**< whether a solution has been passed to GMO */
   );

   /** resolves with discrete variables fixed */
   bool solveFixed();

   /** indicates whether instance is an LP */
   bool isLP();

public:
   /** constructs GamsOsi interface for a specific solver */
   GamsOsi(
      OSISOLVER          solverid_           /**< ID of underlying LP/MIP solver */
   )
   : gmo(NULL),
     gev(NULL),
     opt(NULL),
     msghandler(NULL),
     osi(NULL),
     solverid(solverid_)
   { }

   ~GamsOsi();

   int readyAPI(
      struct gmoRec*     gmo_,               /**< GAMS modeling object */
      struct optRec*     opt_                /**< GAMS options object */
   );

   int callSolver();
};

#endif
