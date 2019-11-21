// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_HPP_
#define GAMSIPOPT_HPP_

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct optRec* optHandle_t;

#include "IpTNLP.hpp"
#include "IpIpoptApplication.hpp"

/** GAMS interface to Ipopt */
class GamsIpopt
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
};

#endif
