// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_HPP_
#define GAMSIPOPT_HPP_

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;

#include "IpTNLP.hpp"
#include "IpIpoptApplication.hpp"
#include "GamsNLP.hpp"

#include <cstdint>

/** GAMS interface to Ipopt */
class DllExport GamsIpopt
{
   friend int main(int, char**);
private:
   struct gmoRec*         gmo;                /**< GAMS modeling object */
   struct gevRec*         gev;                /**< GAMS environment */

   /// whether we are IpoptH or Ipopt
   bool ipoptlicensed;
   /// Ipopt environment
   Ipopt::SmartPtr<Ipopt::IpoptApplication> ipopt;
   /// NLP to be solved
   Ipopt::SmartPtr<GamsNLP> nlp;

   /// whether next solve should be a warmstart (use ReOptimizeTNLP)
   bool warmstart;
   /// information which variable lower and upper bounds are present (bitflags)
   uint8_t* boundtype;

   /// sets up ipopt, allows for gev==NULL
   void setupIpopt();

public:
   GamsIpopt()
   : gmo(NULL),
     gev(NULL),
     ipoptlicensed(false),
     warmstart(false),
     boundtype(NULL)
   { }

   ~GamsIpopt()
   {
      delete[] boundtype;
   }

   int readyAPI(
      struct gmoRec*     gmo_
   );

   int callSolver();

   int modifyProblem();
};

#endif
