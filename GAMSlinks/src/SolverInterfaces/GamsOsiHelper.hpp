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
   OsiSolverInterface&   solver,             /**< OSI solver interface */
   bool                  setupnames          /**< should col/row names be setup in Osi? */
);

/** stores a solution (including dual values and basis status) from OSI in GMO
 * @return true on success, false on failure
 */
bool gamsOsiStoreSolution(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   const OsiSolverInterface& solver          /**< OSI solver interface */
);

/** writes the problem stored in an OSI into LP and MPS files
 * set the first bit of formatflags for using writeMps, the second bit for using writeLp, and/or the third for using writeMpsNative
 */
void gamsOsiWriteProblem(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   OsiSolverInterface&   solver,             /**< OSI solver interface */
   unsigned int          formatflags         /**< in which formats to write the instance */
);

#endif /*GAMSOSIHELPER_HPP_*/
