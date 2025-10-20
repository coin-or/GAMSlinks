// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSCBC_HPP_
#define GAMSCBC_HPP_

#include <cstdlib>
#include <deque>
#include <string>
#include "GamsLinksConfig.h"

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct optRec* optHandle_t;

class CbcModel;
class CbcParameters;
class GamsCbcMessageHandler;
class OsiSolverInterface;

/** GAMS interface to CBC */
class GamsCbc
{
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */

   GamsCbcMessageHandler* msghandler;        /**< message handler */
   CbcModel*             model;              /**< CBC model object */

   double                optcr;              /**< relative optimality tolerance */
   double                optca;              /**< absolute optimality tolerance */
   bool                  usewallclock;       /**< whether the clock type is wallclock time */
   bool                  mipstart;           /**< whether to pass primal solution to MIP solve */
   int                   nthreads;           /**< number of threads to use */
   char*                 writemps;           /**< name of mps file to write instance to */
   bool                  solvefinal;         /**< whether to solve MIP with fixed discrete variables finally */
   char*                 solvetrace;         /**< name of trace file to write solving info to */
   int                   solvetracenodefreq; /**< node frequency for solve trace */
   double                solvetracetimefreq; /**< time frequency for solve trace */
   char*                 dumpsolutions;      /**< name of solutions index gdx file for writing all solutions */
   char*                 dumpsolutionsmerged;/**< name of gdx file for writing all solutions */

   bool setupProblem();

   bool setupStartingPoint();

   bool setupParameters(
      std::deque<std::string>& cbc_args,
      CbcParameters&           cbcParam
   );

   bool writeSolution(
      double             cputime,            /**< CPU time spend by solver */
      double             walltime            /**< wallclock time spend by solver */
   );

   bool isLP();

public:
   GamsCbc()
   : gmo(NULL),
     gev(NULL),
     msghandler(NULL),
     model(NULL),
     optcr(0.0),
     optca(0.0),
     mipstart(false),
     nthreads(1),
     writemps(NULL),
     solvefinal(true),
     solvetrace(NULL),
     solvetracenodefreq(100),
     solvetracetimefreq(5.0),
     dumpsolutions(NULL),
     dumpsolutionsmerged(NULL)
   { }

   ~GamsCbc();

   int readyAPI(
      struct gmoRec*     gmo_               /**< GAMS modeling object */
   );

   int callSolver();
};

extern "C" {

DllExport
int cbcCreate(
   void** Cptr,
   char*  msgBuf,
   int    msgBufLen
   );

DllExport
void cbcFree(
   void** Cptr
   );

DllExport
int cbcCallSolver(
   void* Cptr
   );

DllExport
int cbcReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
   );

} // extern "C"

#endif /*GAMSCBC_HPP_*/
