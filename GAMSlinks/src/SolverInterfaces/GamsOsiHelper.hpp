// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSOSIHELPER_HPP_
#define GAMSOSIHELPER_HPP_

class OsiSolverInterface;
struct gmoRec;

/** loads a problem from GMO into OSI
 * @return true on success, false on failure
 */
bool gamsOsiLoadProblem(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   OsiSolverInterface&   solver              /**< OSI solver interface */
);

/** stores a solution (including dual values and basis status) from OSI in GMO
 * @return true on success, false on failure
 */
bool gamsOsiStoreSolution(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   const OsiSolverInterface& solver          /**< OSI solver interface */
);

#endif /*GAMSOSIHELPER_HPP_*/
