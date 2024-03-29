// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSCOUENNE_HPP_
#define GAMSCOUENNE_HPP_

#include <cstdlib>

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

class GamsMessageHandler;
class CbcModel;
namespace Couenne
{
class CouenneInterface;
class CouenneProblem;
class CouenneSetup;
class expression;
}

/** GAMS interface to Couenne */
class GamsCouenne
{
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct palRec*        pal;                /**< GAMS audit and license object */

   Couenne::CouenneSetup* couenne_setup;     /**< Couenne solver application */
   GamsMessageHandler*   msghandler;         /**< COIN-OR message handler for GAMS */
   bool                  ipoptlicensed;      /**< whether a commercial Ipopt license is available */

   bool isMIP();
   Couenne::CouenneProblem* setupProblem();
   Couenne::expression* parseGamsInstructions(
      Couenne::CouenneProblem* prob,         /**< Couenne problem that holds variables */
      int                codelen,            /**< length of GAMS instructions */
      int*               opcodes,            /**< opcodes of GAMS instructions */
      int*               fields,             /**< fields of GAMS instructions */
      int                constantlen,        /**< length of GAMS constants pool */
      double*            constants           /**< GAMS constants pool */
   );
   /** passes OsiObjects for SOS, semicontinuous, and semiinteger variables to CouenneInterface */
   void passSOSSemiCon(
      Couenne::CouenneInterface* ci,         /**< Couenne interface where to add objects for SOS and semicon constraints */
      CbcModel*          cbcmodel            /**< CBC model used for B&B */
   );

public:
   GamsCouenne()
   : gmo(NULL),
     gev(NULL),
     pal(NULL),
     couenne_setup(NULL),
     msghandler(NULL),
     ipoptlicensed(false)
   { }

   ~GamsCouenne();

   int readyAPI(
      struct gmoRec*     gmo                 /**< GAMS modeling object */
   );

   int callSolver();
};

#endif /*GAMSCOUENNE_HPP_*/
