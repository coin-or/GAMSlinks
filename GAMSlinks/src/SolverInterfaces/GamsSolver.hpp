// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSSOLVER_HPP_
#define GAMSSOLVER_HPP_

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct optRec* optHandle_t;

/** abstract interface to a solver that takes Gams Modeling Objects (GMO) as input */
class GamsSolver
{
public:
   /** sets number of threads to use in basic linear algebra routines */
   static void setNumThreadsBlas(
      struct gevRec*      gev,                /**< GAMS environment */
      int                 nthreads            /**< number of threads for BLAS routines */
   );

   /** ensure that GMO library has been loaded
    * @return nonzero on failure, 0 on success
    */
   static int getGmoReady();

   /** ensure that GEV library has been loaded
    * @return nonzero on failure, 0 on success
    */
   static int getGevReady();

   /** calls GAMS license check, if build by GAMS
    * @return True if license check was skipped or successful or model fits into demo size restrictions
    */
   bool checkLicense(
      struct gmoRec*     gmo                 /**< GAMS modeling object */
   );

   /** calls GAMS academic license check, if build by GAMS
    * @return True if license check was skipped or an academic GAMS license was found or model fits into demo size restrictions
    * If model fits into demo size limitations but also an academic license is available, isdemo is set to false.
    */
   bool checkAcademicLicense(
      struct gmoRec*     gmo,                /**< GAMS modeling object */
      bool&              isdemo              /**< bool to indicate whether check succeeded because model fit into demo size */
   );

   /** registers a GAMS/CPLEX license, if build by GAMS
    * @return True if license was registered or no CPLEX available
    */
   bool registerGamsCplexLicense(
      struct gmoRec*     gmo                 /**< GAMS modeling object */
   );

   /** checks for GAMS/Ipopt commercial license
    * @return True if Ipopt commercial license was found, false otherwise (even for demo models).
    */
   bool checkIpoptLicense(
      struct gmoRec*     gmo                 /**< GAMS modeling object */
   );

   /** initialization of solver interface and solver
    * The method should do initializes necessary for solving a given model.
    * In case of an error, the method should return with a nonzero return code.
    */
   virtual int readyAPI(
      struct gmoRec*     gmo,                /**< GAMS modeling object */
      struct optRec*     opt                 /**< GAMS options object */
   ) = 0;

   /** indicates whether the solver interface and solver supports the modifyProblem call */
   virtual int haveModifyProblem()
   {
      return -1;
   }

   /** notifies solver that the GMO object has been modified and changes should be passed forward to the solver */
   virtual int modifyProblem()
   {
      return -1;
   }

   /** initiate solving and storing solution information in GMO object */
   virtual int callSolver() = 0;
};

#endif /*GAMSSOLVER_HPP_*/
