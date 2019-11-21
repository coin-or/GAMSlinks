// Copyright (C) GAMS Development and others 2016
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSSOPLEX_HPP_
#define GAMSSOPLEX_HPP_

#include <cstdlib>
#include <ostream>

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

namespace soplex
{
class SoPlex;
}

class GamsOutputStreamBuf;

/** GAMS interface to SoPlex */
class GamsSoPlex
{
   friend void printSoPlexOptions();
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct palRec*        pal;                /**< GAMS audit and license object */

   soplex::SoPlex*       soplex;             /**< SoPlex object */
   GamsOutputStreamBuf*  logstreambuf;       /**< output stream buffer to write to log file only */
   std::ostream*         logstream;          /**< output stream to write to log file only */

   void setupLP();
   void setupParameters();
   void writeInstance();

public:
   GamsSoPlex()
   : gmo(NULL),
     gev(NULL),
     pal(NULL),
     soplex(NULL),
     logstreambuf(NULL),
     logstream(NULL)
   { }

   ~GamsSoPlex();

   int readyAPI(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   int callSolver();

   /** notifies solver that the GMO object has been modified and changes should be passed forward to the solver */
   int modifyProblem();
};

#endif /*GAMSSOPLEX_HPP_*/
